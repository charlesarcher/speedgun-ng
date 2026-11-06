# Feature Specification: Hardware Latency Probe with SMI Attribution

**Feature Branch**: `048-hwlat-probe`

**Created**: 2026-11-05

**Status**: Draft

**Input**: User description, opening quoted:

> Hardware latency probe in the style of hwlatdetect, feature N13d of the
> roadmap /workspace/speedgun-ng-harness-roadmap.md (section 10, N13d).
> Short name: hwlat-probe. Target machine: corsairone (Intel Skylake-X,
> i9-9920X, one socket). Lane: counters (corsairone). It has the Intel
> MSR_SMI_COUNT. Goal: The N13a jitter probe finds gaps but cannot tell
> an SMI from an IRQ. N13d adds (1) a user-space probe mode with a low
> threshold and the SMI count from MSR_SMI_COUNT (0x34) and the NMI line
> of /proc/interrupts, and (2) an optional root-only run of the kernel
> hwlat tracer through tracefs, which restores every tracefs setting at
> exit.

The request carries the rulings Q-1 and Q-2 of the lane prompt, given by
the owner on 2026-11-04, and the recommendations Q-3 to Q-5, which the
lane prompt directs the lane to take in place of a clarification round.
They stand in Decisions as D-1 to D-5. The remaining decisions close the
shapes the prompt leaves open.

## Spec identity and run-time binding

- Spec number: `048`. `specs/` on master `858c55d9` holds `001` to `040`
  and `042` to `047`; `041` stands reserved for N6-P and stands absent
  from master. `git ls-remote --heads origin` names no ref carrying
  `048` (read 2026-11-05). `gh pr list --state all` names no head ref
  carrying `048`; the highest open pull request stands at `#80` on head
  `046-report-rows` (read 2026-11-05). `048` stands free and the lane
  takes it.
- Branch: `048-hwlat-probe`, cut from master `858c55d9` on 2026-11-05.
  Master stands on that hash at branch time, so no moved-base record
  applies.
- Version: master stands at `0.31.0` (`CMakeLists.txt:64`). The lane
  takes the next free minor, `0.32.0`. The one open pull request (`#80`,
  head `046-report-rows`) changes one file under `specs/` and reserves no
  version (read 2026-11-05).
- Report schema: master stands at `1.14` (`kSchemaVersion`,
  `source/harness/json_report.cpp:49`). The top-level `x-order` of
  `docs/schema/report-v1.json:7` stands unchanged, because the feature
  adds no top-level key. Every new key joins inside an object the
  document already carries, so the minor moves to the next free value,
  `1.15`, and each touched object's `x-order` moves in the same commit
  (D-6, FR-004, FR-029).
- Harness `SOVERSION`: master stands at `21` (`CMakeLists.txt:419`). The
  feature moves an exported harness layout: the per-cpu gap figures record
  `JitterStats` (`include/speedgun-ng/benchmark.hpp:516`) and the tracer
  records `PrecheckTracerCpuRecord` (`:706`) and `PrecheckTracerRecord`
  (`:725`) gain fields, and the two closed sets `JitterMethod` (`:551`)
  and `TracerEngine` (`:2458`) gain one value each. The number rises to
  `22`, and the commit that writes the fields records the check beside
  the numbers, the shape specs `036` FR-004 and `043` FR-005 state.
- Archive `SOVERSION`: master stands at `6` (`CMakeLists.txt:143`). The
  two new readers live on the harness side under `source/harness/`, and
  no header of the counters library under `include/speedgun-ng/` changes,
  so the number stays `6`. A plan that moves a reader into the counters
  library moves this check with it.
- Option values: the feature adds no option row and no option value.
  `kJitterProbeOptionValue` at `260` and `kOsNoiseOptionValue` at `320`
  (`source/harness/detail/options.hpp:88`, `:96`) stand unchanged, the
  tracer block `321` to `329` stays free, and
  `test/source/harness_cli_table_test.cpp` keeps the name and value
  uniqueness checks over the unchanged table.

## Roadmap position

- Item N13d of the harness roadmap, section 10. The roadmap file stands
  outside this repository and absent on this host (Assumptions), and the
  lane prompt carries the item text.
- The probe spec named this lane when it closed its own scope:
  `specs/027-jitter-probe/spec.md:335` and `:901` list "the hardware
  latency probe and the SMI counts" as the deferred N13d work. This
  feature closes that entry.
- The lane joins one line to the gap-attribution line the roadmap draws
  between the probes and the kernel: N13a (spec `027`) finds a gap, N13b
  (spec `034`) attributes the gap to a kernel counter without root, N13c
  (spec `043`) adds the fixed-quanta methods, and N13e (spec `036`) adds
  the `osnoise` and `timerlat` engines. N13d names the SMI as a source
  and adds the kernel's hardware-latency detector as a third engine.
- Roadmap section 10.6 stays in force in full.

## Lane identity

- The acceptance shape stands on the lane host: corsairone, an Intel
  Skylake-X i9-9920X on one socket, which states the Intel `SMI` counter
  at `MSR_SMI_COUNT`. The host reads as `Intel(R) Core(TM) i9-9920X CPU
  @ 3.50 GHz` in `/proc/cpuinfo` (read 2026-11-05).
- Root is required for one acceptance run of each surface and optional
  everywhere else. The feature states what it can read without root and
  refuses no host for the absence of it (FR-011, FR-023).
- The suite stays host-agnostic: the file root, the clock and the cpu set
  come from injected seams, the tests carry fake files alone, and the
  suite passes on a host without root, without the `msr` nodes and
  without tracefs (FR-001, FR-032).
- Records name the host by its lane name, the kernel and the compiler.
  No record carries a home directory path (`specs/032-fix-merged-features`
  FR-024).

## Audit point

- Repository: `master` at `858c55d9` (project version `0.31.0`, report
  schema `1.14`, harness `SOVERSION 21`, archive `SOVERSION 6`), read
  2026-11-05 on the lane host, kernel `7.3.0-4-cachyos`, GCC `16.2.1`.
- The `--jitter-probe` value grammar stands at
  `specs/027-jitter-probe/contracts/cli-options.md:23`, and the
  `method=` row at `:35` names the three standing words `tsc`, `fwq` and
  `ftq`. The parser stands at `source/harness/jitter_probe.cpp:1665`
  (the `method` key), and one method stands per run.
- The kept gap record stands at `source/harness/detail/jitter_probe.hpp:41`
  (`GapRecord`: `stamp`, `gapNs`), with the size invariant
  `static_assert(sizeof(GapRecord) == 16)` at `:47`. The per-cpu gap
  figures stand public at `include/speedgun-ng/benchmark.hpp:516`
  (`JitterStats`, sixteen fields). The document carries the pair under
  the `jitter` object: `stats[]` per probe cpu with `gaps[]` items of
  `stamp` and `gapNs` (`docs/schema/report-v1.json`). The `gaps[]` array
  stands written under the `--json-raw=gaps` gate alone
  (`source/harness/json_report.cpp:354`).
- The attribution figures the pre-check reads today stand at
  `include/speedgun-ng/benchmark.hpp:652` (`PrecheckAttribution`:
  `interrupts`, `softirqs`, `switches`, `schedules`), and the readers
  stand at `source/harness/jitter_probe.cpp:1811` (`interruptsOfText`),
  `:1845` (`softirqsOfText`), `:1885` (`schedstatOfText`), `:1939` and
  `:1987`. The pre-check reads `/proc/interrupts` today at `:2055`
  through `readUnderRoot`, and `interruptsOfText` sums every labeled row
  of the cpu column, so an `NMI` count stands folded into the standing
  `interrupts` figure on a host that states the row. No reader parses the
  `NMI` label, and no reader of any `msr` node stands in the tree
  (checked 2026-11-05: no source file names `/dev/cpu`, `MSR_SMI_COUNT`
  or `rdmsr`).
- The counters library reaches the same counter through the vendor
  performance tables, where `msr@smi@` backs the `smi_num` and
  `smi_cycles` metrics
  (`external/pmu-events/arch/x86/alderlake/adl-metrics.json:60`). That
  path goes through the kernel's perf interface. FR-007's reader goes
  through the `msr` node, and the two paths stand independent.
- The file-root seam stands at `source/detail/proc_root.hpp:35` and
  `:52` (`procRootPath`, `installTestProcRoot`), landed by spec `034` and
  reused by spec `036`; every `/proc` and `/sys` read of the harness
  resolves under it (specs/034 FR-019, FR-020, specs/036 R-01).
- The tracer lifecycle stands at `source/harness/detail/noise_tracer.hpp`
  (`NoiseTracerSession` at `:45`, `noiseTracerSetup`,
  `noiseTracerFigures`, `noiseTracerRestore`) and
  `source/harness/noise_tracer.cpp`. The setup reads the standing
  `current_tracer` word and the standing `tracing_cpumask` text, writes
  the engine word, the mask and `tracing_on` at `1`, and the restore
  writes `tracing_on` at `0`, then the two saved values (specs/036
  FR-007, R-04). The session saves two values today; the hwlat engine
  writes four more files, so the saved set grows with it (D-8).
- The tracer lifecycle holds no destructor, no `atexit` hook and no
  signal restore: `noiseTracerRestore` (`source/harness/noise_tracer.cpp:375`)
  writes three values, drops each write result, and the call site
  (`source/harness/cli.cpp:1250-1259`) stands right behind the probe
  start's latch, so no early return sits between the setup and the
  restore today. The batch owns one signal handler, `SIGINT`, and it only
  sets a flag (`cli.cpp:69-72`, `:1056`). A setup that fails on its mask
  write or its `tracing_on` write returns the empty session with
  `current_tracer` already rewritten and nothing restored
  (`noise_tracer.cpp:313-355`). No test sends a signal to the tracer.
- The standing figure read names one path for both engines,
  `sys/kernel/tracing/osnoise/cpu<id>` (`noise_tracer.cpp:367`), so a
  third engine names its own result files under the same root. The
  engine refusal text lists the two standing words
  (`noise_tracer.cpp:55-56`), so a third word moves that line and its
  test.
- The docs surfaces a new word touches: `docs/pages/harness.md:1827`
  (`## Jitter probe`, 396 lines, four tables), `:2223`
  (`### The noise tracer`, prose plus a five-row name table),
  `README.md:723`, `:736`, `:749` (the three capability sections) and
  `README.md:964`, `:965` (the two option rows). `README.md` stands at
  1,001 lines against `PAGE_LINE_CAP = 1004`, three lines of headroom.
  The drift gate scans `README*`, `AGENTS.md` and `docs/**/*.md`, and its
  forbidden-token scan bars a spec id, a requirement id, a commit hash
  and a kernel reference from the two user-guide pages.
- The gate state on this host at `858c55d9`, each exit measured:
  `python3 tools/docs/example_drift.py .` 0, `cmake -P cmake/lint.cmake`
  0, `cmake -P cmake/spell.cmake` 0, `bash test/time_source_gate.sh .` 0,
  `python3 tools/prose/prose_gate.py --check prose --mode tree` 0 over
  1,011 sources, and the two DBC gates 0 with 347 interfaces and 0 gaps.
  The prose range form needs a base that stands behind HEAD; with
  `origin/master` equal to HEAD it examines no source and its 0 says
  nothing.
- The standing coverage and suite figures: 84 coverage-exclusion marker
  lines in the tree, 0 of them under `source/harness/`; the coverage
  gate `tools/dbc/coverage_gate.sh` at 100.0% lines and a branch
  shortfall allowance of 21 rows; `ctest -N` at 181 tests; the taskset
  lane `taskset -c 0,1 ctest --preset prepush-quick -j 2`; the newest
  recorded host reading 99.5% lines and 98.8% branches with 108 dark
  branches
  (`specs/047-loader-cross-field/results/acceptance-2026-11-03.md:100`).
- The comparison reads no pre-check figure today: `kJitterFields` holds
  32 rows (`source/harness/compare.cpp:262`), the field-count word
  `thirty-two` stands in the refusal text at `:429` and in
  `specs/027-jitter-probe/contracts/rule-names.md:31`, and
  `compare.cpp` never reads `PrecheckRecord`. A new comparable name
  moves the row, the count word in both places, and the contract line in
  one commit.
- Two word tables move with the two new enumerators:
  `kJitterMethodWords` (`source/harness/detail/json_report.hpp:89`) and
  `kTracerEngineWords` (`:103`), each closed by a `static_assert` on the
  enum, beside the loader's word list and the schema's enum list.
- The engine word of the `--os-noise` value parses at
  `source/harness/noise_tracer.cpp:48` and names `osnoise` or `timerlat`
  today; the grammar stands at
  `specs/036-osnoise-timerlat-tracers/contracts/cli-options.md:31`.
- The host facts read on this box on 2026-11-05, as an unprivileged
  user: `/dev/cpu/0/msr` exists and answers `EACCES`; `/proc/interrupts`
  carries 83 lines and no `NMI` line, because the kernel withholds the
  `NMI` and `LOC` rows from an unprivileged reader;
  `/sys/kernel/tracing` exists, its files answer `EACCES`, and no
  `hwlat_detector` directory stands under it while the detector stands
  unloaded. Each state is the ordinary not-available path of this
  feature, and each one the tests fake.
- The docs gates stand at `tools/docs/example_drift.py`
  (`PAGE_LINE_CAP = 1004` at `:256`; `README.md` stands at 1001 lines on
  master), `tools/prose/prose_gate.py`, `tools/dbc/dbc_pair_gate.py`,
  `cmake/lint.cmake` and `cmake/spell.cmake`. The guide page stands at
  `docs/pages/harness.md` (2,714 lines).
- The results-evidence precedent stands at `specs/036-osnoise-timerlat-tracers/results/`
  and `specs/035-numa-placement/results/`: one record per run naming the
  settings, the commands and the exit codes.

## Standing facts this lane reads

- The standing probe of spec `027`, the pre-check of spec `034` and the
  two quanta methods of spec `043` stand unchanged. The new method joins
  the `method=` word set and adds reads beside the standing gap loop.
- The standing tracer of spec `036` keeps its setup, its figure read and
  its restore, and the hwlat engine joins it as a third engine word. The
  pre-check window, the tracer cpu rule and the not-available rule of
  spec `036` stand for the new engine unchanged.
- The option-table contract of
  `specs/015-benchmark-harness-core/contracts/cli.md` stands unchanged:
  no row joins it. The lane amends nine standing contract files across
  four features, each row named by the contract file of this feature that
  owns it, and each amendment landing in the commit that carries the code
  it describes (FR-039). The set: spec `027` gains the `method=` row, the
  report rows and the grown field table; spec `026` gains the rule
  production for the new head word; spec `034` gains the probe figure
  rows for the report and the console; spec `036` gains the engine word,
  the tracer report rows and the tracer console lines.
- The report contract of spec `034`
  (`specs/034-host-noise-precheck/contracts/report-json.md`) and of spec
  `036` (`specs/036-osnoise-timerlat-tracers/contracts/report-json.md`)
  carry the two records this lane widens.
- The rule-name contract of spec `027`
  (`specs/027-jitter-probe/contracts/rule-names.md`) carries the
  comparison names, and this lane adds names beside them.

## Decisions

- **D-1 (owner ruling, 2026-11-04; Q-1)**: the user-space mode is one
  word of the standing key. `--jitter-probe` takes `method=hwlat` beside
  `tsc`, `fwq` and `ftq`, and that method carries the SMI read and the
  NMI read. No option value joins the table.
- **D-2 (owner ruling, 2026-11-04; Q-2)**: the kernel run is one word of
  the standing option. `--os-noise` takes `hwlat` beside `osnoise` and
  `timerlat`, and the engine reuses the tracefs lifecycle and the
  restore code of spec `036`. No option value joins the table.
- **D-3 (Q-3, taken)**: the SMI read goes through
  `/dev/cpu/<cpu>/msr` alone. The feature never loads a module and never
  writes a model-specific register. A missing or unreadable node answers
  not available.
- **D-4 (Q-4, taken)**: a part without a documented SMI counter answers
  not available. On AMD parts the counter at `0x34` stands undocumented
  across the family, so the SMI read answers not available there. The
  feature names no counter it cannot cite.
- **D-5 (Q-5, taken)**: the version takes the next free minor, `0.32.0`,
  and the schema minor moves to `1.15`.
- **D-6**: one word, two surfaces. `hwlat` names a method of
  `--jitter-probe` in one grammar and an engine of `--os-noise` in the
  other grammar. The method reads the two counters in user space; the
  engine runs the kernel detector. Each surface documents on its own
  line, and no console line, key or test mixes the two.
- **D-7**: a gap carries the SMI change of the span that ends at it. A
  gap stands known only after it ended, so no read can precede it. The
  probe reads the counter once on the gap path and once at each window
  edge, and the change of a gap stands as this read minus the read before
  it: the opening edge read for the first gap of a window, and the read of
  the previous gap for every later gap. The mark names the span between
  two reads, and the guide page states that span rule. The read rides the
  gap path, so the standing loop keeps its cost (FR-012, FR-013, FR-014).
- **D-8**: the saved set of the hwlat engine holds one value per file it
  saves: `current_tracer`, `tracing_cpumask`, `hwlat_detector/mode`,
  `hwlat_detector/width`, `hwlat_detector/window` and `tracing_thresh`.
  The restore writes each saved value back. The spec `036` session shape
  carries two values today and grows to carry the set (FR-019, FR-021).
  `tracing_on` stands outside the set: the restore writes `0`, the
  standing shape the spec `036` tests assert, and a run that stopped a
  tracer that stood on leaves tracing off (FR-021). The kernel saves
  `tracing_thresh` when the engine word lands and writes that saved value
  back at its reset (`kernel/trace/trace_hwlat.c:836`, `:857`), so the
  kernel restores the run's own threshold and only the session holds the
  host's. The kernel also records the run's threshold as the default a
  later run takes when it names none
  (`kernel/trace/trace_hwlat.c:855`), and the guide page states that
  residue.
- **D-9**: the kernel creates `hwlat_detector/` at boot, so the run never
  waits for the directory. `kernel/trace/trace_hwlat.c:774` creates it at
  init and puts `window` (`:778`), `width` (`:785`) and `mode` (`:792`)
  inside it, each readable and writable by the owner, and it carries no
  `enable` file: the engine word of `current_tracer` reaches the tracer's
  start (`kernel/trace/trace_hwlat.c:842`) and `tracing_on` starts the
  sampling. The run writes `mode` with the word `per-cpu`
  (`kernel/trace/trace_hwlat.c:67`), so each tracer cpu carries its own
  detector thread and states figures of its own. The kernel default
  `round-robin` moves one thread across the mask, and a mask of many cpus
  then leaves most of them with no window of their own. The detector reads
  `tracing_cpumask` and `tracing_thresh` when it starts
  (`kernel/trace/trace_hwlat.c:434`, `:836`), and its bound compares each
  width write against the standing window
  (`kernel/trace/trace_hwlat.c:734`), so the mask, the mode, the two knobs
  and the threshold stand written before the engine word. The order stands
  in the requirement (FR-019).
- **D-10**: the three values the run writes stand named constants, and
  the run states them in the console block and the record. The lane
  prompt names no values. The lane's proposal holds a window of
  10,000 us, a width of 5,000 us and a trigger threshold of 1,000 us:
  the width stands below the window, the kernel's rule, and the window
  stands twenty times inside the 200 ms pre-check default, so one default
  run leaves each tracer cpu about twenty windows of its own under the
  per-cpu mode of D-9 (Assumptions A-3). The kernel's own defaults and
  bounds stand cited beside A-3 and FR-019.
- **D-11**: the user-space method keeps the standing threshold rule of
  spec `027`: 200 ns, rising to ten times the measured loop period. The
  key `threshold=` stands, and a reader who wants a low threshold types
  it. The method's own difference is the two counter reads and the
  marks, so the lane changes no default (FR-002, A-4).
- **D-12**: a failed restore is a disclosure with a name. The run states
  one line per file whose restore write failed, naming the file and the
  value the run saved at setup, so an operator can put that file back by
  hand. The status stays the status of the run (FR-022, the standing rule
  of specs/036 FR-016).
- **D-13**: the new figures ride the records the run already writes. The
  per-cpu gap figures record carries the two counter deltas and the
  marked-gap count, and the tracer's per-cpu record carries the three
  detector figures. No new top-level key joins the document, and no new
  record joins the loaded report (FR-025).
- **D-14**: the attribution never decides the run's status. The marks and
  the deltas inform the reader; they refuse nothing and fail nothing, the
  way the pre-check attribution of spec `034` behaves.
- **D-15**: the restore covers every path the process controls. The
  standing lifecycle restores after a complete setup only: a setup that
  fails partway leaves `current_tracer` rewritten, and the batch's one
  signal handler sets a flag and returns. The hwlat run owns one saved
  value per file it wrote, so the fix rides the shared lifecycle, and the
  two standing engines gain the same restore on the same defect path. The
  lane amends the spec `036` contract line for that path in the same
  commit. A stop the process cannot control, `SIGKILL` among them,
  outruns every handler: the record names each file the run wrote and the
  value it saved, so an operator can put the host back by hand.

## Out of scope

- Any counter other than `MSR_SMI_COUNT` and the `NMI` line of
  `/proc/interrupts`.
- An AMD SMI counter. D-4 closes the source, and the feature names no
  substitute for it.
- A setuid helper, a capability grant, a daemon, or a module load. The
  run uses the rights the process already holds (FR-024).
- A standalone clone of hwlatdetect: a second binary, its own option
  surface, or a run outside the batch.
- Writes to any file outside the tracefs files FR-019 names, and any
  write to any `msr` node.
- New figures for `osnoise` and `timerlat`, and any change to their
  restore order. The shared restore fix of D-15 reaches their defect path
  as a consequence, and the lane records that consequence beside the spec
  `036` contract line.
- Storing a whole trace: the run reads each tracer cpu's buffer text once
  at the end of the window, folds three figures from it, and keeps no
  buffer of its own.
- A periodic background sampler of the SMI count. D-7 bounds the reads
  to the gap path and the window edges.
- Any platform outside Linux, and any CI change.
- Spec `041` (N6-P), which stands reserved and absent from master.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - A gap names the SMI that stole the cpu (Priority: P1)

A performance engineer runs the batch with
`--jitter-probe=idle,method=hwlat`. The probe keeps its gaps above the
threshold. Each gap now carries the change of the SMI count across the
span that ends at it, and the per-cpu figures carry the change of the SMI
count and of the NMI count across the run, with the count of gaps the run
marked SMI. The engineer sees which gaps firmware caused.

**Why this priority**: the request names this as the feature's reason to
exist: the standing probe finds a gap and cannot tell an SMI from an
interrupt.

**Independent Test**: with an injected file root that carries a fake
`msr` node and a fake `/proc/interrupts`, a run whose fake counter rises
between the two reads that bound one gap marks that gap SMI, a run whose
counter stands flat marks it not SMI, and the report carries both
figures. The run needs no kernel change and no root.

**Acceptance Scenarios**:

1. **Given** a file root whose fake `msr` node rises by one between the
   two reads that bound one gap, **When** the run carries `method=hwlat`,
   **Then** that gap carries the mark SMI, the per-cpu figures carry the
   SMI change one and the marked-gap count one, and the console states
   the same figures.
2. **Given** a file root whose fake `msr` node stands flat across every
   gap, **When** the run carries `method=hwlat`, **Then** every gap
   carries the mark not SMI and the marked-gap count stands 0.
3. **Given** a file root with no `msr` node, **When** the run carries
   `method=hwlat`, **Then** the console states `SMI count not available`
   once, every gap carries no mark, the run continues, and the exit
   status stands 0.
4. **Given** the same run without root on the lane host, **When** the
   console block prints, **Then** the SMI figures and the NMI figure
   read `not available` and the standing gap figures stand unchanged.

### User Story 2 - The kernel detector runs for the run (Priority: P2)

A root user runs the batch with
`--jitter-probe=idle,mode=precheck --os-noise=hwlat`. The run writes the
detector's settings, measures across the pre-check window, reads the
detector's figures, and puts every setting back at the value the run
read before it. The user sees the gaps the kernel records with its own
interrupts masked, and the host stands as it stood.

**Why this priority**: the second surface of the request. It carries the
one figure the user-space probe cannot reach: a gap measured with the
kernel holding interrupts off.

**Independent Test**: with an injected fake tracefs tree, a run saves
`current_tracer`, `tracing_cpumask`, `hwlat_detector/mode`,
`hwlat_detector/width`, `hwlat_detector/window` and `tracing_thresh`,
writes them with the run's values, folds the three figures from the fake
per-cpu buffer text, and leaves every saved file at the value the tree
held before the run on all three exit paths: the normal end, a failed
write mid-run, and a signal that ends the run.

**Acceptance Scenarios**:

1. **Given** a fake tracefs tree whose files hold known values, **When**
   the run carries `--os-noise=hwlat`, **Then** the tree holds the
   engine word in `current_tracer` during the window, holds each of the six
   saved files at its value from before the run, and holds `tracing_on`
   at `0`.
2. **Given** the same tree, **When** the detector's per-cpu buffer text
   states a longest gap, a gap count and an NMI count, **Then** the
   console and the record carry those three figures for each tracer
   cpu.
3. **Given** a run whose setup write fails partway, **When** the run
   ends, **Then** every file the run wrote stands restored and the
   tracer lines state the failure.
4. **Given** a run that takes `SIGINT` during the window, **When** the
   run ends, **Then** every file the run wrote stands restored, and the
   record states the path the run took.
5. **Given** a host with no tracefs, no rights, or no detector, **When**
   the run carries `--os-noise=hwlat`, **Then** the tracer lines state
   `not available`, the user-space pre-check stands whole, and the exit
   status stands 0.

### User Story 3 - The report states what the probe cannot see (Priority: P2)

A reader of the report learns that the user-space probe runs with the
cpu's interrupts enabled. The console block and the document state the
limit in one line, so a reader reads a marked or unmarked gap with the
right confidence, and the reader learns the detector surface as the way
to see a gap with interrupts held off.

**Why this priority**: an unmarked gap can come from an interrupt
handler. Without the limit line, the attribution invites a wrong
reading, and the request names the limit as a deliverable.

**Independent Test**: a run of the user-space method prints the limit
line once and writes the limit into the document, and a run that names
neither new surface writes neither.

**Acceptance Scenarios**:

1. **Given** a run with `method=hwlat`, **When** the probe block prints,
   **Then** one line states that the user-space probe runs with
   interrupts enabled, and a reader gets the detector as the second
   surface.
2. **Given** a run with neither new surface, **When** the document
   loads, **Then** the document carries no new key.

### User Story 4 - A bad value refuses before anything runs (Priority: P2)

A user mistypes a word. The batch refuses with one quoted line and exit
status 2 before any benchmark runs, the standing shape of the option
surface.

**Why this priority**: the refusal shape is the standing contract of
both grammars, and the two new words join it.

**Independent Test**: for each invalid value of the two error tables, the
run prints one line naming the fault and exits 2 with no benchmark
output.

**Acceptance Scenarios**:

1. **Given** `--jitter-probe=idle,method=smi`, **When** the batch
   starts, **Then** one line names the method word and the exit status
   stands 2.
2. **Given** `--jitter-probe=idle,method=hwlat,method=hwlat`, **When**
   the batch starts, **Then** one line names the repeated key and the
   exit status stands 2.
3. **Given** `--os-noise=hwlato`, **When** the batch starts, **Then**
   one line names the engine word and the exit status stands 2.
4. **Given** `--os-noise=hwlat` on a run whose mode runs no pre-check
   window, **When** the batch starts, **Then** one line names the
   missing pre-check window and the exit status stands 2.

### User Story 5 - The docs teach both surfaces (Priority: P3)

A reader of the guide page learns the method word, the engine word, the
two figures, the not-available rule, the privilege rule and the limit.
The example program runs, and its expected output stands guarded by the
shape gate.

**Why this priority**: the docs gates bind every feature, and a page
that drifts from the binary fails the drift gate.

**Independent Test**: the example builds and runs, the `.expected` file
folds equal against the run, the page block matches its region, and the
prose gate reports no finding on the new section.

**Acceptance Scenarios**:

1. **Given** the guide page's new section, **When** the drift gate runs,
   **Then** the page block folds equal against the example's run.
2. **Given** the README rows of the two options, **When** the drift gate
   reads the page, **Then** the page stands at or under 1,004 lines.

### User Story 6 - The acceptance runs stand recorded (Priority: P3)

The lane records two root runs on corsairone: one of the user-space
method and one of the detector. Each record names the host, the kernel,
the compiler, the commands and the exit codes, and the detector record
carries the state of every tracefs file before and after the run.

**Why this priority**: the record is the evidence a later lane reads,
and the restore claim is only as good as its before and after readings.

**Independent Test**: the results directory holds one record per run, the
quickstart links each record, and each record carries the before and
after readings of every file the run wrote.

**Acceptance Scenarios**:

1. **Given** the two acceptance runs, **When** the lane closes, **Then**
   each record names the commands, the exit codes and the figures, and
   the detector record names each file's value before and after.
2. **Given** the recorded evidence, **When** the quickstart reads,
   **Then** it links every record.

### Edge Cases

- No `msr` node, an unreadable node, or a short read: the SMI figures
  read `not available`, the console states it once per run, and the run
  continues with status 0.
- A part that states no documented SMI counter, an AMD part among them:
  the same disclosure, and the feature names no substitute counter.
- No `NMI` line in `/proc/interrupts`, which is what an unprivileged
  reader gets on the lane host: the NMI figure reads `not available` and
  the SMI figures stand on their own.
- A gap at the last instant of the window, where the gap's read stands
  past the window's end: the gap carries the change of the span that ends
  at it, and the window delta stays the change of the two edge reads.
- A gap the probe's own read opens: the span of that gap holds no SMI
  change, so the gap reads `NOT_SMI`, and the guide page states the rule.
- A gap count above the buffer limit: the standing dropped-gap figures
  of spec `027` carry the loss, and the marked count counts kept gaps
  only.
- A probe cpu whose `msr` node stands readable while another cpu's node
  does not: each cpu's figures answer their own node.
- No tracefs mount, no rights for the writes, or no detector in the
  kernel: the tracer lines state `not available` and status 0, the
  standing rule of spec `036`.
- A write that fails after an earlier write stood: the run restores the
  files it wrote, states the failure on the tracer lines, and keeps
  status 0.
- A `SIGINT` inside the window: the flag path reaches the restore, each
  written file stands restored, and the record states the path it took.
- A stop the process cannot control, `SIGKILL` among them: no handler
  runs, the settings stand changed, and the record carries the file list
  and the saved values an operator needs to undo them (D-15).
- A setup that fails after one write: the run restores that file, answers
  the not-available shape, and keeps status 0.
- A restore write that fails: one line per file names the file and the
  value the run saved, and the run keeps its status (D-12).
- A pre-check window shorter than the detector window: the gap count
  answers the detections the buffer holds, which can stand zero, and the
  record states the window, the width and the threshold the run wrote.
- A run that names `--os-noise=hwlat` with a `--jitter-probe` method
  word of `hwlat`: both surfaces run, each records on its own keys, and
  the figures of the two never merge into one figure.
- A document written before this feature: `loadReport` reads it with no
  new key and no complaint.

## Requirements *(mandatory)*

FR-001 to FR-006 carry the lane prompt's six requirements, with their
numbers unchanged. FR-007 onward state the rules those six depend on.

### The lane's six (FR-001 to FR-006)

- **FR-001**: When a path names a root-only source (`/dev/cpu/<cpu>/msr`,
  `/proc/interrupts`, or tracefs), the feature SHALL resolve it under the
  injectable file root that specs `034` and `036` use. Every test SHALL
  use fake files.
- **FR-002**: The user-space mode SHALL stand as one key of
  `--jitter-probe`: the word `hwlat` of the `method=` key, beside `tsc`,
  `fwq` and `ftq`. No option value SHALL join the option table unless the
  owner rules.
- **FR-003**: The detector run SHALL stand off by default. It SHALL
  require the engine word `hwlat` of `--os-noise`. A value that names an
  engine on a run whose mode plans no pre-check window SHALL refuse with
  one line at exit status 2 ahead of any benchmark. It SHALL change no
  kernel setting without that word, SHALL restore each setting on every
  exit path, and SHALL carry no setuid helper.
- **FR-004**: The console, the JSON document and the N5 comparison SHALL
  carry the new figures: per cpu the SMI count change, the NMI count
  change and the count of gaps marked SMI, and for the detector the
  longest gap, the gap count and the NMI count of its samples. The schema
  minor SHALL move by one. `loadReport` SHALL read the new figures back, and a report
  written without them SHALL load unchanged.
- **FR-005**: An invalid value SHALL exit with status 2 and one quoted
  line, before any benchmark runs. The engine-word refusal text names the
  accepted words, so it SHALL name the third word, and the method-word
  refusal text SHALL name `hwlat` the same way.
- **FR-006**: The lane SHALL document the feature in a section of
  `docs/pages/harness.md`, in `docs/schema/report-v1.json`, and in one
  example under `example/docs/` with its `.expected` file. `README.md`
  SHALL stay at or under 1,004 lines.

### The SMI read and the NMI read (FR-007 to FR-012)

- **FR-007**: When the probe runs the `hwlat` method on a cpu, the probe
  SHALL read `MSR_SMI_COUNT` at address `0x34` from
  `/dev/cpu/<cpu>/msr`, resolved under the file root, as one 8-byte
  little-endian unsigned value, and SHALL never write that file.
- **FR-008**: The feature SHALL load no kernel module and SHALL change no
  module parameter.
- **FR-009**: When the probe runs the `hwlat` method on a cpu, the probe
  SHALL read the `NMI` line of `/proc/interrupts`, resolved under the
  file root, and SHALL take the count of that cpu's column. The standing
  `interrupts` figure SHALL keep its standing sum over every labeled row
  of the column, so the new figure stands beside it with its own name.
- **FR-010**: When the running part states no documented SMI counter, the
  SMI figures SHALL answer not available, and the feature SHALL name no
  substitute counter. The gate SHALL read the `vendor_id` line of
  `/proc/cpuinfo` through the file root and SHALL take its answer from the
  text it reads, so a test plants the text it needs (D-4).
- **FR-011**: When a counter source stands missing, unreadable or
  unparsable, the figures of that source SHALL answer not available, the
  console SHALL state the standing wording once per run for that source,
  the run SHALL continue, and the exit status SHALL stand 0.
- **FR-012**: The probe SHALL read the SMI counter once per kept gap and
  once at each window edge, and no other read SHALL stand on the loop
  path. The standing gap loop SHALL keep its standing cost, and the kept
  gap record SHALL keep its 16-byte size and its standing fields.

### The attribution of one gap (FR-013 to FR-017)

- **FR-013**: When the probe keeps a gap above the threshold, the probe
  SHALL read the SMI counter and SHALL carry the change of that read over
  the read before it, the opening edge read for the first gap of a window
  and the read of the previous gap for every later gap (D-7).
- **FR-014**: When a kept gap's SMI change stands above zero, the gap
  SHALL carry the mark SMI. When the change stands at zero, the gap SHALL
  carry the mark not SMI. The mark answers the span between the two reads,
  and the guide page states that rule.
- **FR-015**: When the SMI count stands unavailable, no gap of the run
  SHALL carry a mark, and the run SHALL keep every standing figure.
- **FR-016**: The per-cpu figures of a probe cpu SHALL carry the change of
  the SMI count across that cpu's window, the change of the NMI count
  across the same window, and the count of kept gaps marked SMI. The
  window stands the pre-check window in the pre-check phase and the
  probe's own run span in the alongside phase, and each delta answers the
  two reads at its edges. The console SHALL print the three figures in
  the standing shape of the block that prints them: the pre-check block
  prints one source-named line per cpu, and the probe block prints one
  line per figure with one column per probe cpu. A figure that answers
  nothing names its source file in the standing disclosure line.
- **FR-017**: The NMI change SHALL stand as a disclosed figure. The mark
  of one gap SHALL answer the SMI counter alone, and no figure of this
  feature SHALL change the run's exit status (D-14).

### The detector engine (FR-018 to FR-024)

- **FR-018**: The engine set of `--os-noise` SHALL take `hwlat` beside
  `osnoise` and `timerlat`, and the engine SHALL reuse the standing
  setup, window, cpu rule and restore shape of spec `036`. The standing
  figure read names the `osnoise` count-file path for both engines it
  serves, and no such file stands for the detector, so the new engine
  SHALL read the per-cpu buffer text
  `sys/kernel/tracing/per_cpu/cpu<id>/trace` under the same file root and
  SHALL take its three figures from the detector's own lines (A-6).
- **FR-019**: When the detector engine starts, the feature SHALL read the
  standing value of each file it saves. Before the first write the session
  reads `current_tracer`, and when that text already reads `hwlat` the
  session writes nothing and answers the shape FR-023 names: the kernel
  returns success without initializing a tracer that already stands
  (`kernel/trace/trace.c:5043` to `:5044`), so it clears no buffer, and
  the live detector's lines stand outside this run (P-15). With that read
  answered by any other word, the session SHALL write `tracing_cpumask`
  with the tracer cpu set, the standing step, then SHALL
  write `hwlat_detector/mode` with the word `per-cpu`, then SHALL write
  `hwlat_detector/width` and `hwlat_detector/window` with the two values
  FR-020 names, the width first, then SHALL write `tracing_thresh` with
  the threshold in nanoseconds, then SHALL write `current_tracer` with the
  engine word, then SHALL write `tracing_on` at `1`, each path resolved
  under the file root (D-9).
- **FR-020**: The window, the width and the threshold SHALL stand as
  named constants, the width below the window, in microseconds, and the
  console block and the record SHALL state all three (D-10).
- **FR-021**: When the detector run ends on any path the process
  controls, the feature SHALL write `tracing_on` at `0` and SHALL write
  each saved value back to the file it came from, in the reverse of the
  FR-019 order, so the threshold goes back after the engine word: the
  normal end, a read or write that fails partway through the setup, a read
  or write that fails inside the window, and the standing `SIGINT` path. When the setup fails after it wrote one or more
  files, the run SHALL restore each file it wrote before it answers the
  not-available shape (D-15).
- **FR-022**: When a restore write fails, the feature SHALL state one line
  naming the file and the saved value, and SHALL keep the run's exit
  status. The lifecycle drops each restore result today, so this line
  stands new console output on the standing rule that the run never fails
  on the tracer.
- **FR-023**: When no tracefs stands at the standing path, the detector
  stands absent, the process holds no rights for the writes, or
  `current_tracer` already reads `hwlat` ahead of the run, the tracer
  lines SHALL state `not available`, one disclosure line SHALL name that
  last cause, the user-space figures SHALL stand whole, and the exit
  status SHALL stand 0.
- **FR-024**: The feature SHALL gain no setuid helper, no capability
  grant, no daemon, and no write outside the files FR-019 names.

### The record, the reader and the comparison (FR-025 to FR-029)

- **FR-025**: The per-cpu gap figures record SHALL carry three new
  figures: `smiDelta`, `nmiDelta` and `smiGapCount`. The writer SHALL
  write them for a run of the `hwlat` method at every place that record
  stands in the document: the run-level `jitter.stats[]`, the per-entry
  `jitter.stats[]` items, and `precheck.cpus[].stats`. In a per-entry item
  the two delta figures SHALL answer `null` and the count figure SHALL
  answer 0, because that fold covers one benchmark's windows and holds no
  window edge. The writer SHALL write no key of the three for a run of any
  other method.
- **FR-026**: Each kept gap the document writes under the standing
  `--json-raw=gaps` gate SHALL carry, for the `hwlat` method, the mark key
  `smi` with the word `SMI` or `NOT_SMI`, and SHALL carry `null` where the
  SMI count stands unavailable.
- **FR-027**: The tracer record SHALL take `hwlat` as an engine word, and
  SHALL carry the three settings the run wrote as `windowUs`, `widthUs`
  and `thresholdUs`. Each per-cpu tracer entry of the `hwlat` engine SHALL
  carry `maxGapNs`, `gapCount` and `nmiCount`, each `null` where the
  buffer of that cpu holds no detector line. Under the per-cpu mode of
  D-9 each tracer cpu carries its own buffer, so a cpu the detector
  watched and found nothing on holds no line and answers `null`. A zero
  answers the `nmiCount` figure of a cpu whose lines carry no `nmi-count`
  field. The console block SHALL print the three figures
  for each tracer cpu, in the shape the standing tracer lines use, and
  SHALL print the three settings beside them.
- **FR-028**: The comparison SHALL carry one rule name for each of the six
  new figures. The three probe figures SHALL join the standing jitter
  field table, which grows from 32 rows to 35, and SHALL read the carrier
  the standing jitter names read. The three detector figures SHALL stand
  behind a new rule head word, `hwlat`, in the shape
  `hwlat:<cpu|any>:<figure>` with the standing direction and threshold
  suffix, and SHALL read the tracer record of the
  loaded report, the first pre-check figure the comparison reads. A
  figure the record carries as absent SHALL read 0, the standing rule of a
  fold that carries no value. A report that carries no record at all, or
  no entry for a cpu a rule names, SHALL answer the standing fault line at
  exit status 2, and the comparison SHALL gain no new status. The field-count word of the jitter refusal line and of the
  rule-name contract moves with the new rows, and the new head word gains
  its own refusal line and its own contract rows.
- **FR-029**: The schema file SHALL carry every new key, in the order the
  writer writes it, with its value kind. A document written before this
  feature SHALL load with no new key, and a run that names neither new
  surface SHALL write the same bytes master writes, apart from the schema
  and version values.

### The limits line (FR-030, FR-031)

- **FR-030**: When the user-space method runs, the console block SHALL
  state once that the probe runs with the cpu's interrupts enabled, so a
  gap can come from an interrupt handler.
- **FR-031**: The document SHALL carry the same statement as the run's
  limit text under the `limits` key of the `jitter` object, beside the
  attribution figures, and the guide page SHALL state the difference
  between the two surfaces: privilege, threshold unit and interrupt
  state.

### The seams and the tests (FR-032 to FR-035)

- **FR-032**: Every test of this feature SHALL read fake files through the
  standing file-root seam, SHALL take its time from the standing fake
  provider and the scripted window reader, and SHALL name no cpu the host
  lacks. The standing fakes carry the shapes this feature needs: a path
  that stands a directory answers a refused open, and a lowered file-size
  limit answers a refused write.
- **FR-033**: No test SHALL sleep and no test SHALL read a wall clock, and
  the time-source gate SHALL keep its exit 0 over the lane's files.
- **FR-034**: Each attribution case, each not-available case and each
  restore path SHALL stand as one named test.
- **FR-035**: Each test SHALL stand in the same commit as the code it
  tests.

### The documentation and the gates (FR-036 to FR-039)

- **FR-036**: `docs/pages/harness.md` SHALL carry the method word in the
  jitter section and the engine word in the tracer subsection, each in the
  form the page's standing sections use: one purpose sentence, one marked
  snippet where the section carries one, one result sentence, and the
  field names the new figures print. The two sections SHALL state three
  rules a reader cannot read off the figures: a mark answers the span
  between two counter reads, the detector runs one thread per tracer cpu
  with interrupts off for its width, and a restore leaves the kernel's own
  remembered threshold at the run's value. The page's forbidden-token rule bars
  a spec id, a requirement id, a commit hash and a kernel reference from
  the added lines.
- **FR-037**: `example/docs/` SHALL carry one example program with its
  `.expected` file, registered as a docs example target and a shape-check
  entry, and the drift gate SHALL pass on the page block, the example
  region and the README rows the lane moves. `README.md` SHALL stay at or
  under 1,004 lines, and the teaching of the two new words SHALL ride the
  guide page, because the main page keeps three lines of headroom.
- **FR-038**: The prose gate, the spell gate, the lint gate, the DBC pair
  gate and the drift gate SHALL pass on the lane's diff.
- **FR-039**: The nine standing contract files this lane amends SHALL
  carry the new words in the commit that lands the code each row
  describes: `cli-options.md`, `report-json.md` and `rule-names.md` of
  spec `027`; `rule-parse.md` of spec `026`; `report-json.md` and
  `console-report.md` of spec `034`; and `cli-options.md`,
  `report-json.md` and `console-report.md` of spec `036`. Each contract
  file of this feature names the standing line and file it amends
  (FR-029).

## Key Entities *(include if feature involves data)*

- **SMI reading**: one cpu's count of system-management interrupts, from
  `MSR_SMI_COUNT` at `0x34`, held as an unsigned 64-bit count or as not
  available. Attributes: cpu index, value, availability.
- **NMI reading**: one cpu's count from the `NMI` line of
  `/proc/interrupts`. Attributes: cpu index, value, availability.
- **Gap attribution**: one kept gap's mark. Attributes: the gap's stamp
  and length, the SMI change of the gap's window, the mark `SMI`, `NOT_SMI`
  or no mark.
- **Probe cpu figures**: the per-cpu figures the run adds beside the
  standing gap figures: `smiDelta`, `nmiDelta`, `smiGapCount`.
- **Detector run**: one kernel-detector measurement across the pre-check
  window. Attributes: the engine word, the three settings the run wrote,
  one entry per tracer cpu.
- **Detector cpu figures**: the three figures the detector states for one
  cpu: `maxGapNs` from the longer of the kernel's two sample figures,
  `gapCount` from the sample lines of the cpu, and `nmiCount` as their
  NMI count total, each nullable.
- **Setting snapshot**: one file's saved value and its path, one entry per
  file the run saves, six entries with `tracing_on` outside the set, the
  restore's whole input.
- **Disclosure**: one not-available statement, naming its source, printed
  once per run per source.
- **Limits statement**: the run's text that the user-space probe runs with
  interrupts enabled.

The new report keys, with the figure each carries and the test that
guards it. Two write sites fill the three stats shapes
(`source/harness/json_report.cpp:332` to `:364` and the inline pre-check
block at `:1242` to `:1262`), so the three figures reach the per-entry
items too. The schema types that per-entry
`jitter` object open (`docs/schema/report-v1.json:572-574`), so it needs
no order row:

| Key | Object | Written when | Read back by | Test |
| --- | --- | --- | --- | --- |
| `HWLAT` word | `jitter.method` | `method=hwlat` | `loadReport` | `harness_hwlat_probe_test.cpp` |
| `smiDelta` | `jitter.stats[]`, `precheck.cpus[].stats` | `hwlat` method | `loadReport` | `harness_hwlat_probe_test.cpp` |
| `nmiDelta` | `jitter.stats[]`, `precheck.cpus[].stats` | `hwlat` method | `loadReport` | `harness_hwlat_probe_test.cpp` |
| `smiGapCount` | `jitter.stats[]`, `precheck.cpus[].stats` | `hwlat` method | `loadReport` | `harness_hwlat_probe_test.cpp` |
| `smi` | `jitter.stats[].gaps[]` | `hwlat` method with `--json-raw=gaps` | `loadReport` validates the kind | `harness_hwlat_probe_test.cpp` |
| `hwlat` word | `precheck.tracer.engine` | `--os-noise=hwlat` | `loadReport` | `harness_hwlat_tracer_test.cpp` |
| `windowUs` | `precheck.tracer` | `--os-noise=hwlat` | `loadReport` | `harness_hwlat_tracer_test.cpp` |
| `widthUs` | `precheck.tracer` | `--os-noise=hwlat` | `loadReport` | `harness_hwlat_tracer_test.cpp` |
| `thresholdUs` | `precheck.tracer` | `--os-noise=hwlat` | `loadReport` | `harness_hwlat_tracer_test.cpp` |
| `maxGapNs` | `precheck.tracer.cpus[]` | `--os-noise=hwlat` | `loadReport` | `harness_hwlat_tracer_test.cpp` |
| `gapCount` | `precheck.tracer.cpus[]` | `--os-noise=hwlat` | `loadReport` | `harness_hwlat_tracer_test.cpp` |
| `nmiCount` | `precheck.tracer.cpus[]` | `--os-noise=hwlat` | `loadReport` | `harness_hwlat_tracer_test.cpp` |
| `limits` | `jitter` | `hwlat` method | `loadReport` | `harness_hwlat_probe_test.cpp` |

Eleven keys stand new. Two enum word sets gain one value each: the method words of the document
(`TSC`, `FWQ`, `FTQ`, `HWLAT`) and the engine words (`NONE`, `osnoise`,
`timerlat`, `hwlat`), whose first word answers the `NONE` enumerator and
stands in no written document (`source/harness/detail/json_report.hpp:100`
to `:102`). No key changes its kind, and no key moves.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: One test covers each attribution case (a gap with an SMI
  change, a gap without one, a host whose SMI count stands
  unavailable), one test covers each not-available case (no `msr` node,
  an unreadable node, no `NMI` line, a part with no documented counter,
  no tracefs, no rights, no detector), and one test covers each restore
  path (normal, failed write, signal), all on fake files. The count
  stands 13 tests or more, and each passes.
- **SC-002**: The lane's coverage stands 100% line coverage, with a
  branch shortfall of 21 or less stated as a number, no new exclusion
  marker of any kind, and a marker count of 84 or less in the tree.
- **SC-003**: The full suite passes on corsairone, and
  `taskset -c 0,1 ctest --preset prepush-quick -j 2` passes there too,
  both counts recorded beside the 181 tests of record. No test sleeps,
  reads a wall clock or names an absent cpu, and the time-source gate
  exits 0.
- **SC-004**: Each gate exits 0 on the lane's diff:
  `cmake -P cmake/lint.cmake`, `cmake -P cmake/spell.cmake`,
  `python3 tools/docs/example_drift.py .`, `bash test/time_source_gate.sh .`,
  the prose gate over the lane's range with a base that stands behind
  HEAD, and the DBC pair gate with the document matrix. A range gate whose
  base equals HEAD examines no source and states nothing. The release
  build, the analyzer build, the sanitizer lane, the thread-sanitizer lane
  and the coverage contract also exit 0, Principle VIII's set.
- **SC-005**: Two root runs stand recorded on corsairone, one per surface,
  each with its host, kernel, compiler, commands, exit codes, figures,
  and, for the detector run, the value of every written file before and
  after.
- **SC-006**: A run that names neither new word writes a document whose
  bytes match master's document apart from the schema and version values,
  measured by the standing byte test.
- **SC-007**: After the recorded root detector run, each file the run
  saved stands at the value the record names for it before the run:
  `current_tracer`, `tracing_cpumask`, `hwlat_detector/mode`,
  `hwlat_detector/width`, `hwlat_detector/window` and `tracing_thresh`,
  measured on all six files, with `tracing_on` standing 0 as the standing
  restore leaves it.
- **SC-008**: The marked-gap count of a fake run equals the count of gaps
  whose span holds an SMI change, measured on each of the three fake
  counter shapes of SC-001.
- **SC-009**: `README.md` stands at or under 1,004 lines with the new rows
  in place, and the drift gate reports no finding.
- **SC-010**: A reader of the guide page finds one section per surface:
  measured by 2 sections, 1 example, 1 `.expected` file, and 0 prose-gate
  findings on the lane's added lines.

## Constraints

- Prose: ASD-STE100 Simplified Technical English under constitution XI,
  in every line the lane adds, including commit text and records.
- Requirements: EARS form under constitution III, with the pre- and
  postcondition discipline of constitution II on every new function.
- The kept gap record keeps its 16 bytes and its two fields
  (`source/harness/detail/jitter_probe.hpp:47`), so the ring buffer and
  the probe's hot path keep their standing cost (constitution VII).
- The two new readers stand on the harness side and add no dependency and
  no public counters header.
- The feature reads root-only sources through the standing file-root seam
  alone, the way specs `034` and `036` do.
- The run changes no kernel setting without the engine word, and no path
  leaves a setting changed without a named disclosure.
- Linux only. No CI change. No new option row, option value or short
  letter.
- The lane rebases only, carries no merge commit, squashes to one commit
  before the merge, and measures acceptance on the squashed head. Commit
  author and committer stand as the owner's identity, titles at 50
  characters or fewer, body lines at 72 or fewer, and the footer's last
  two lines stand as `Refs:` and `Approved-by:` trailers.
- No person's name appears in the repository outside the `LICENSE`
  copyright line, and no record carries a home directory path.

## Assumptions

- **A-1**: The roadmap file `/workspace/speedgun-ng-harness-roadmap.md`
  stands outside this repository and absent on this host, so the lane
  prompt's section-10 text of N13d stands as the item's wording.
- **A-2**: On the lane host, the `msr` module stands loaded, so
  `/dev/cpu/<cpu>/msr` nodes exist and answer an unprivileged open with
  `EACCES` (read 2026-11-05). A host without the nodes takes the
  not-available path.
- **A-3**: The three values the detector run writes stand the lane's
  proposal, held as named constants: a 10,000 us window, a 5,000 us width
  and a 1,000 us threshold. The kernel's own defaults stand 1,000,000 us
  for the window and 500,000 us for the width
  (`kernel/trace/trace_hwlat.c:53`, `:54`), and 10 us for the threshold
  (`kernel/trace/trace_hwlat.c:55`). A one second window finds no whole
  window inside the pre-check's default span, so the lane keeps the
  kernel's 1:2 ratio at a window twenty times inside that span, and a
  threshold above the kernel default keeps a short interruption of the
  sampling loop out of the reported gaps. This host carries no
  hwlatdetect install to read (checked 2026-11-05). The owner can re-rule
  the three at plan time without moving any requirement.
- **A-4**: `method=hwlat` keeps the standing threshold rule of spec `027`,
  and a reader lowers the threshold with the standing `threshold=` key
  (D-11).
- **A-5**: The kernel withholds the `NMI` line of `/proc/interrupts` from
  an unprivileged reader on the lane host, measured 2026-11-05, so the
  not-available path stands the ordinary non-root path.
- **A-6**: The kernel's detector writes one line per detected gap into
  the per-cpu ring-buffer text at `sys/kernel/tracing/per_cpu/cpu<id>/trace`,
  in the shape `kernel/trace/trace_output.c:1531` prints, and names no
  per-cpu statistics file. The lane reads that text for its three
  figures, and the tests carry fake files of the same shape.
- **A-7**: `loadReport` treats a key the document omits as an absent
  figure, the standing shape of an optional field in the loader.
- **A-8**: The owner's rulings of 2026-11-04 (Q-1, Q-2) stand, and the
  recommendations Q-3 to Q-5 stand taken as D-3, D-4 and D-5.

## Constitution Checklist

- Main-page impact check (IV.1): yes. The feature changes what the
  harness teaches, so the lane carries its README rows, its guide-page
  section and its example region in the same change (FR-006, FR-036,
  FR-037, SC-009, SC-010). The reading of IV.1 the lane follows stands
  recorded: the root README carries the two option rows inside its
  1,004-line cap, and the teaching lands on `docs/pages/harness.md`,
  because the file stands three lines under that cap today and the guide
  page is where specs `036` and `043` put the teaching of their surfaces.
