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
- Version: the branch cut at `0.31.0` and named `0.32.0`, which
  specs/041-counter-probe landed on master while the branch stood. Master
  stands at `0.33.0` (`CMakeLists.txt:64`, read 2026-11-06), so the lane
  takes `0.34.0`.
- Report schema: master stands at `1.15` (`kSchemaVersion`,
  `source/harness/json_report.cpp:58`). This branch cut at `1.14` and
  named `1.15` for its own move, which specs/041-counter-probe landed on
  master first, so the eleven keys take the next minor. The top-level
  `x-order` of `docs/schema/report-v1.json:7` stands unchanged, because
  the feature adds no top-level key. Every new key joins inside an object
  the document already carries, so the minor moves to the next free value,
  `1.16`, and each touched object's `x-order` moves in the same commit
  (D-6, FR-004, FR-029).
- Harness `SOVERSION`: master stands at `21` (`CMakeLists.txt:419`). The
  feature moves an exported harness layout: the per-cpu gap figures record
  `JitterStats` (`include/speedgun-ng/benchmark.hpp:516`) and the tracer
  records `PrecheckTracerCpuRecord` (`:706`) and `PrecheckTracerRecord`
  (`:725`) gain fields, and the two closed sets `JitterMethod` (`:551`)
  and `TracerEngine` (`:2458`) gain one value each. The number rises to
  `22`, and the commit that writes the fields records the check beside
  the numbers, the shape specs `036` FR-004 and `043` FR-005 state.
- Archive `SOVERSION`: master stands at `8` (`CMakeLists.txt:173`), the
  number specs/050-limit-table-lifecycle landed. The three new readers
  live on the harness side under `source/harness/`, and no header of the
  counters library under `include/speedgun-ng/` changes, so the number
  stands. A plan that moves a reader into the counters library moves this
  check with it.
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
- The host facts read on this box as an unprivileged user:
  `/dev/cpu/0/msr` exists and answers `EACCES` (read 2026-11-05);
  `/proc/interrupts` carries 83 lines and no `NMI` line, because the
  kernel withholds the `NMI` and `LOC` rows from an unprivileged reader
  (read 2026-11-05); `/sys/kernel/tracing` exists as
  `drwx------ root root` (read 2026-11-07), so a listing answers
  `EACCES` and names nothing under the mount. The host builds the
  detector in: `CONFIG_HWLAT_TRACER=y` stands in its kernel
  configuration, `init_hwlat_tracer` calls `init_tracefs` at
  `kernel/trace/trace_hwlat.c:883` behind the `late_initcall` at `:887`,
  and that path creates `hwlat_detector/` at `:774` with `window`
  (`:778`), `width` (`:785`) and `mode` (`:792`) inside it. The
  directory stands there from boot and reads only with the rights. Each
  unprivileged state is the ordinary not-available path of this feature,
  and each one the tests fake.
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
- **D-5 (Q-5, taken)**: the version takes the next free minor, `0.34.0`,
  and the schema minor moves to `1.16`, one past the `1.15` that
  specs/041-counter-probe took while this branch stood.
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
  then leaves most of them with no window of their own. The per-cpu start
  this lane rules reads `tracing_cpumask` at
  `kernel/trace/trace_hwlat.c:585`, and the tracer's init reads
  `tracing_thresh` at `:836`. The detector's bound compares each width
  write against the standing window (`kernel/trace/trace_hwlat.c:734`),
  so the mask, the mode, the two knobs and the threshold stand written
  before the engine word. The order stands in the requirement (FR-019).
- **D-10**: the three values the run writes stand named constants, and
  the run states them in the console block and the record. The lane
  prompt names no values. The lane's proposal holds a window of
  10,000 us, a width of 5,000 us and a trigger threshold of 1,000 us:
  the width stands below the window, the lane's stricter bound that FR-020
  states, and the window stands twenty times inside the 200 ms pre-check
  default, so one default run leaves each tracer cpu about twenty windows
  of its own under the per-cpu mode of D-9 (Assumptions A-3). The kernel
  refuses a value below its minimum or above its maximum and accepts a
  width equal to the window (`kernel/trace/trace.c:6728` to `:6732`). The
  kernel's own defaults and bounds stand cited beside A-3 and FR-019.
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
detector's figures, and puts each saved file back at the value the run
read before it. The user sees the gaps the kernel records with its own
interrupts masked. After the restore the six saved files hold the
values the record names, `tracing_on` stands `0`, and the kernel keeps
the run's threshold as the standing default a later run takes when it
names none (D-8).

**Why this priority**: the second surface of the request. It carries the
one figure the user-space probe cannot reach: a gap measured with the
kernel holding interrupts off.

**Independent Test**: with an injected fake tracefs tree, a run saves
`current_tracer`, `tracing_cpumask`, `hwlat_detector/mode`,
`hwlat_detector/width`, `hwlat_detector/window` and `tracing_thresh`,
writes them with the run's values, folds the three figures from the fake
per-cpu buffer text, and leaves every saved file at the value the tree
held before the run on the three exit paths these scenarios test: the
normal end, a setup write that fails partway, and a signal that ends
the run.

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
   ends, **Then** the six saved files stand at the values the tree held
   before the run, `tracing_on` stands `0`, and the tracer lines state
   the failure.
4. **Given** a run that takes `SIGINT` during the window, **When** the
   run ends, **Then** the six saved files stand at the values the tree
   held before the run, `tracing_on` stands `0`, and the record states
   the path the run took.
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
- A `hwlat_detector/mode` read form that carries brackets, the form the
  kernel prints on every read: the saved value stands the bracketed word,
  and the restore writes that word back (FR-019, P-16).
- A write that fails after an earlier write stood: the run restores the
  files it wrote, states the failure on the tracer lines, and keeps
  status 0.
- A `SIGINT` inside the window: the flag path reaches the restore, the
  six saved files stand at the values the record names, `tracing_on`
  stands `0`, and the record states the path it took.
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

- **FR-001**: When a path names a kernel source (`/dev/cpu/<cpu>/msr`,
  `/proc/interrupts`, `/proc/cpuinfo`, or tracefs), the feature SHALL
  resolve it under the injectable file root that specs `034` and `036`
  use, root-only or not. Every test SHALL use fake files.
- **FR-002**: The user-space mode SHALL stand as one key of
  `--jitter-probe`: the word `hwlat` of the `method=` key, beside `tsc`,
  `fwq` and `ftq`. The method keeps the gap loop of `tsc`, so the sample
  series record of specs/043 SHALL stand absent for it, as it stands
  absent for `tsc` (D-1). No option value SHALL join the option table
  unless the owner rules.
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
  standing value of each file it saves. The read form of
  `hwlat_detector/mode` lists the three words and marks the active one
  inside brackets (`kernel/trace/trace_hwlat.c:629-632`, `:642`), and the
  write handler compares one stripped word (`:692`, `:694`, `:706-711`),
  so the value the run saves, states and writes back for that file SHALL
  stand the bracketed word alone (P-16). Before the first write the session
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
  status SHALL stand 0. The lane host builds the detector in, so the
  absent-detector shape stands a fake-tree case: corsairone never shows
  it, and the recorded root run cannot demonstrate it.
- **FR-024**: The feature SHALL gain no setuid helper, no capability
  grant, no daemon, and no write outside the files FR-019 names.

### The record, the reader and the comparison (FR-025 to FR-029)

- **FR-025**: The per-cpu gap figures record SHALL carry three new
  figures: `smiDelta`, `nmiDelta` and `smiGapCount`. The writer SHALL
  write them for a run of the `hwlat` method at every place that record
  stands in the document: the run-level `jitter.stats[]`, the per-entry
  `jitter.stats[]` items, and `precheck.cpus[].stats`. In a per-entry item
  the two delta figures SHALL answer `null`, because that fold covers one
  benchmark's windows and holds no window edge, and the count figure SHALL
  fold the changes of that item's own kept gaps, so the counts of the items
  add to the run-level figure the way the standing counts of that item
  add. The writer SHALL write no key of the three for a run of any
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
  no tracefs, no rights, no detector, and `current_tracer` already
  reading `hwlat`), one test covers each restore path (the normal end,
  a write that fails inside the setup, a read or write that fails
  inside the window, and `SIGINT`), and one test covers each gap of the
  two edge cases (the gap whose read stands past the window end, and
  the gap the probe's own read opens), all on fake files. The count
  stands 17 tests or more, and each passes.
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
  after. The host builds the detector in, so the absent-detector shape
  of FR-023 stands outside both records: corsairone never shows it, and
  no recorded run can demonstrate it.
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
- The kept gap record keeps its 16 bytes
  (`source/harness/detail/jitter_probe.hpp:47`) and its two fields
  (`:43-44`), so the ring buffer and the probe's hot path keep their
  standing cost (constitution VII).
- The three new readers stand on the harness side and add no dependency
  and no public counters header.
- The feature reads every kernel source through the standing file-root
  seam alone, root-only or not, the way specs `034` and `036` do.
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


---

## Acceptance extension: Intel final acceptance (2026-11-07)

### Identity and binding

This section extends this spec, `specs/046-hunter-topdown` and
`specs/041-counter-probe` with one records-only acceptance phase. It adds no
feature, and it opens no new spec number. It gathers local evidence on the
Intel host. No reader has to read a spec line or a CI job as proof. A record that
runs "stand recorded" proves nothing until a run stands behind it.

| Line | Value at the check on 2026-11-07 |
|---|---|
| Base head | `615c0964547e4e9768b8e2f2d79991033a0eb5bd`, and `origin/master` at the same head |
| Branch | `docs/intel-final-acceptance`, from the latest `origin/master`; the branch stood absent on the remote |
| Host | corsairone, Intel Core i9-9920X (Skylake-X), one socket, 24 cpus online 0-23 |
| Version | 0.34.0, schema `1.16`, harness SOVERSION 22, archive SOVERSION 8 |
| Page and marker caps | README 1,002 lines against the cap of 1,004; 84 `LCOV_EXCL` markers against the cap of 84 |
| Open pull requests | none at the check |
| Root rights | absent for this session: `sudo` asks for a password, `/sys/kernel/tracing` stands `drwx------ root`, `/dev/cpu/*/msr` stands `crw------- root` |

The recheck of every line above stands at branch time in the record.

### Changed files

The branch changes only files under the three spec directories named above. It
adds no source, test, build, schema, version, CI, gate or tool change, and it
does not touch `.specify/feature.json`. The changed and added files are the three
new records, the nine artifact files of the acceptance phase in the same three
directories, `specs/048-hwlat-probe/quickstart.md` and
`specs/048-hwlat-probe/checklists/intel-final-acceptance.md`. Nothing outside the
three spec directories enters the commit, and the prompt file at the repository
root stays out of it. New records:

- `specs/048-hwlat-probe/results/acceptance-final-intel.md`
- `specs/046-hunter-topdown/results/acceptance-final-intel.md`
- `specs/041-counter-probe/results/acceptance-final-intel.md`

### User Story 1 - A reader can tell a measured root run from a denied path (Priority: P1)

A reader opens the hwlat record and sees which runs this host measured, which
runs it could not measure, and which kernel state the run put back. The reader
never has to guess whether a blocked row is a pass.

**Why this priority**: the record is the only evidence a later lane reads, and
this host withheld root rights from the lane that wrote the standing claim.

**Independent Test**: the record names a command, an exit code and an output for
every row it calls measured, and names the missing right for every row it calls
blocked.

**Acceptance Scenarios**:

1. **Given** a host that refuses tracefs to an unprivileged reader, **When** the
   acceptance runs, **Then** the record states the available source and the
   unavailable source from a real run, and states the blocked root rows.
2. **Given** a root session an operator approves, **When** the detector run
   finishes, **Then** the record compares the six saved values one by one and
   states the value `tracing_on` holds after the run.

### Requirements - the hwlat part (IA48-001 to IA48-006)

- **IA48-001**: The lane SHALL read SC-005, SC-007, FR-019 to FR-024 and the
  current quickstart before it runs anything. The lane SHALL then record the
  facts of the host: the cpu identity, the access to the `msr` node, the tracefs
  mount, the presence of the detector, the kernel configuration and the active
  tracer. Each inspection SHALL be a read, and no inspection SHALL change the
  state it reads.
- **IA48-002**: The lane SHALL run the unprivileged surfaces first, and SHALL
  record for each the source disclosure, the exit status and the round trip of
  the report. The lane SHALL build and run the existing hwlat example and SHALL
  record the output the host answered. The lane SHALL run the three
  refusal commands the quickstart names, each with its quoted line and its exit
  status.
- **IA48-003**: The lane SHALL run the two root surfaces of SC-005 only inside a
  privileged session an operator approves: the `method=hwlat` surface, and the
  explicit pre-check surface with `--os-noise=hwlat`. The lane SHALL use the
  commands of the current quickstart, a benchmark name the build validates, and
  cpus taken from the affinity the host allows. The lane SHALL NOT escalate on
  its own, collect a password, edit a sudoers file, load a module or weaken a
  permission. Where root stands absent, the lane SHALL name the operator action
  it needs and SHALL mark both rows blocked, while the unprivileged acceptance
  continues. A blocked row SHALL NOT be recorded as a pass.
- **IA48-004**: Before the first privileged write, the lane SHALL confirm that no
  unrelated tracer session stands active, and SHALL stop the row where one
  stands active. The lane SHALL snapshot the six files D-8 names -
  `current_tracer`, `tracing_cpumask`, `tracing_thresh`, `hwlat_detector/mode`,
  `hwlat_detector/width` and `hwlat_detector/window` - SHALL read `tracing_on`
  beside them and keep it as a reading of its own, outside the six, and SHALL
  state the mode as the active word the quickstart helper prints, with no
  brackets. The lane SHALL save the console
  text, the document, the exit code and the state the run left behind.
- **IA48-005**: The lane SHALL compare the six saved values of SC-007 one by one,
  once after a normal end and once after the controlled interruption. The
  contract leaves `tracing_on` at 0, so the record SHALL state that reading and
  SHALL NOT claim that the whole machine stands restored. The record SHALL check
  the threshold-default side effect the corrected artifacts describe. Any state
  change the contract does not name SHALL stand as a failed row, and the lane
  SHALL NOT edit the code or the contract to clear it.
- **IA48-006**: A real SMI may not happen. The lane SHALL record a zero SMI count
  as a measured figure and SHALL NOT present it as proof of live SMI
  attribution. Where the `msr` node refuses the read, the lane records no count
  and names the refusal in place of the zero. The planted-SMI claim SHALL rest on the recorded fixture tests.
  The lane SHALL NOT induce a firmware fault or invent a hardware event, and
  SHALL keep hardware evidence apart from fixture evidence.

### Part D - combined local acceptance and records (IA-END-001 to IA-END-003)

- **IA-END-001**: The lane SHALL run the full suite, the full suite under
  `taskset -c 0,1`, the address and undefined-behavior lane, the thread
  sanitizer lane, and the full coverage and gate commands. The record SHALL give
  the pass, fail, skip and not-run counts, the line, function and branch
  figures, the cold rows, the shortfall and each exit code. The thresholds stand
  100 percent lines and branches under the accepted exceptions of
  `specs/045-merge-fixes` FR-015, its R-1, R-2 and W-1, with the tree-wide
  branch allowance at 21. No CI result stands as acceptance,
  and no base failure stands relabeled as green. The lane SHALL inspect the
  affinity and the online cpus first and SHALL NOT invent a cpu id. Where cpus 0
  and 1 stand unpallowed, the record SHALL mark that lane blocked, and a run on
  two other cpus stands as extra evidence and not as a substitute. The lane SHALL
  also run on four allowed cpus. No unit test sleeps or reads a wall clock, and
  hardware acceptance stands apart from unit testing. Where a full coverage run
  cannot finish, the record SHALL name the test, the command, the elapsed time
  and the missing trace, and SHALL accept no partial union and raise no unit
  timeout in silence. Where the standing suppression hides inconsistent data,
  the record SHALL state that the G-25 repair stands pending.
- **IA-END-002**: The lane SHALL run the format, spell, prose, drift, time-source,
  DBC document and DBC pair gates, the analyzer build and the consumer-release
  build, the install audits of the CI test job, the pmu-events integrity check and
  the committed-record check. The record SHALL name the compiler and the python dependencies instead
  of disabling a check. README SHALL stay at or under 1,004 lines and the marker
  count SHALL stay at or under 84.
- **IA-END-003**: Each record and the pull request body SHALL give the host, the
  cpu model, the kernel, the compiler, gcov and lcov versions, the source base
  sha, the measured squashed-head sha, the commands, the counts, whether a root
  session stood available and which rows stand blocked. Paths SHALL stand
  build-relative or `$HOME`-relative, and no path SHALL name a user's home
  directory. The lane SHALL tick only rows that carry evidence.

### Success Criteria - this extension

- **SC-IA-1**: Every row of the three records carries a command with its exit
  code and its output, or names itself blocked with the right it lacks. A row
  without output carries no completion mark.
- **SC-IA-2**: The hwlat record separates three states that a single word would
  blur: a measured root run, a denied unprivileged path, and a blocked row that
  no session on this host could run.
- **SC-IA-3**: The six saved tracefs values of SC-007 appear as six separate
  rows, each with the value before the run and the value after it, with
  `tracing_on` stated at 0 where a root run measured it. Where a row stands
  blocked, `tracing_on` is named as the contract value of FR-021 and not as a
  reading, and the row carries no figure.
- **SC-IA-4**: The hunter record maps every reading to an existing acceptance
  criterion of specs/046 and names the vendored table the mapping came from.
- **SC-IA-5**: The probe record gives the probe limit and the decoded usable
  limit for the Intel non-hybrid core device, with the watchdog rule applied
  exactly once and the source of the published count named.
- **SC-IA-6**: The counts of Part D come from runs on this host at the measured
  head, and the record states each exit code.

### Constraints - this extension

- G-25: the removal of inconsistent suppression and fix data stands required.
  This branch reports it and changes no gate.
- G-29: only the two `group_io` closed-enum switch branch markers stand
  approved. No broader exclusion, no new omit, ignore or `LCOV_EXCL` marker and
  no coverage allowance change.
- G-26 and G-27: old footers, authors and history stay. The new commit carries
  OmO as author and as committer. No person name enters tracked prose outside
  `LICENSE`, and unrelated old records stay as they are.
- G-23: the branch squashes to one commit before the final measurement, rebases
  and never merges. Where a records-only amend follows, the record discloses the
  original and final shas with the exact diff, reruns at the final head and puts
  the final counts in the pull request body. No commit carries its own sha, and
  no pre-squash reading stands called final acceptance.
- The title stands at or under 50 characters, body lines at or under 72, in
  ASD-STE100 Simplified Technical English. The footer block carries, in order,
  `Refs: specs/048-hwlat-probe`, `Refs: specs/046-hunter-topdown`,
  `Refs: specs/041-counter-probe` and `Approved-by: OmO`.
- The first push stands `git push -u origin docs/intel-final-acceptance`. A
  later push saves and verifies the last observed full remote sha, then uses
  `git push --force-with-lease=docs/intel-final-acceptance:<expected-full-sha>`.
  A failed lease stops the lane, and the lane never refreshes it and retries.
  After each push the lane compares `git ls-remote origin
  refs/heads/docs/intel-final-acceptance` with `git rev-parse HEAD` and stops on
  a difference.
- The branch opens one pull request and never merges it and never closes an
  issue.
- G-28 trace-line parsing, G-31 raw codes and G-32 automatic ranks stand future
  work outside this branch. The current catalog-only and explicit-rank behavior
  stays unchanged.
- A later master can supply the coverage-data repair. The lane closes that row
  only by fetching and rebasing onto a master that carries the G-25 removal and
  the fault repair, then rerunning the local commands on that head. A file name
  or a CI pass alone proves nothing, and no lane waits on a named host.

### Assumptions - this extension

- One session on this host owns the working tree for the length of the lane, so
  no sibling lane edits the three spec directories at the same time.
- The build trees `build`, `build/coverage`, `build/sanitize`, `build/tsan`,
  `build/analyzer`, `build/consumer-release` and `build/consumer-release-dev`
  belong to this lane for the length of the runs, and each instrumented run
  stands alone without a parallel writer.
- A blocked row names the missing right and the operator action in plain words, and it
  offers no figure.
- The quickstart stays the source of the two root commands, and the record
  quotes that command and writes no new command.


### Clarifications

#### Session 2026-11-07, round 1

Three readers read this section against the tree, the CI workflow and the code, and
each named the points two careful executors could read apart. The owner ruled on all
of them here. The readers' reports stood outside the repository, so each ruling
carries the evidence a later reader needs.

**The hwlat part.**

1. **Root absent.** With no privileged session, both root rows stand blocked with no
   figure, and SC-005 and SC-007 stand named as unmet by this lane. No row inherits a
   value from a fixture or from another head. The six-row table still appears, with
   its six names and its value cells empty.
2. **Six rows, and one statement.** The record gives six before-and-after rows, and
   states beside them that `tracing_on` stands 0 after the run and that the restore
   never writes its prior value back. The saved mode value carries no brackets, in the
   snapshot and in the record alike.
3. **The quickstart commands.** The record quotes the quickstart commands without the
   `sudo` prefix and names the operator action it needs. It adds no `cpus=` key. The
   tracer cpu set comes from the header line the run printed, because the default rule
   already reads the affinity.
4. **The engine without rights.** The `--os-noise=hwlat` run without root is in scope
   as the denied path: `not available`, one disclosure line, whole user-space figures,
   exit 0. It carries no tracer figure, is never called a detector run, and its exit 0
   is never a pass of the engine.
5. **The controlled interruption.** A root session that exists gets its own interrupted
   run, its own signal command, its own exit status and its own six rows. With no root
   session the row is blocked like the others. The forked-child test closes the code
   path only, and never the host.
6. **The two missing records.** `quickstart.md` names `results/user-space.md` and
   `results/detector.md`, and neither file exists. The branch corrects that sentence in
   the same pass, because it stands under this spec directory.
7. **The threshold residue.** The row is documentary. It names the residue, quotes
   D-8, plan R-4 and the guide page, and states that no user-space read reaches it.
   The residue is named by the contract, so it stands outside the failed-row clause.
8. **The tracer check.** The lane reads `current_tracer` and `tracing_on`. The row
   stops where `current_tracer` reads a word other than `nop`, or where `tracing_on`
   reads 1. A stopped row names the file, the value and the fact that no run executed;
   it carries no console shape and no status.
9. **Which example, and what a round trip is.** The lane runs both artifacts and labels
   each with the target or command it used: the `docs_hwlat_probe` example, and the
   quickstart's `benchmark_example` command. The round trip closes with a
   `--compare=<a>,<b>` run, which loads both documents through `loadReport`. A python
   key print is a parser read and is recorded as that. The committed `.expected` block
   belongs to another host, so no row claims it reproduces here.
10. **The final sha.** The record labels each reading with the head it ran at. After the
    squash it discloses the original and final shas with the exact diff, reruns at the
    final head, and states the rerun per row. A row that cannot rerun says so.

**The hunter part.**

11. **The denominator.** The three counts cover the printed rows, and the record states
    the arithmetic and the `events=` summary line beside them. The 20-row split of AP-9
    is a different measurement and never merges into that figure.
12. **The area cells.** The record prints the `area=` cell texts it saw, with a count
    each, and at least one multi-name path where the run prints one. Where every cell
    holds one name, the record states that as a finding.
13. **The table named.** The record names the selected directory as the block spells
    it, the 13 contributing files with the digests the block prints, and the note that
    the top-down tree arrives through `skx-metrics.json` while the event rows come from
    the other files.
14. **The level.** The record quotes the block's own `level=1` line, names the mapper's
    ask level as a cited constant, and gives the deepest level of the vendored table
    from the table. No row claims the run read a level no line prints.
15. **The base.** The lane runs both hunt commands of the standing record, three times
    each, at the measured head, and builds no old head. The commands keep
    `--hunt-budget 120s`, and the record names the base re-run as not run.
16. **The pass-method disclosure.** The record quotes the plan line with its exit code,
    states whether any method word stood in the command, and states that a hunter run
    prints no companion pass-method line and no coverage line.

**The counter-count part.**

17. **The probe figure.** The `probe=` figure of the `counter_probe` example is the
    lane's probe reading: it is the example's own search in the shipped attribute shape.
    The library pays no open on this host, because its decode answers, and the record
    says so. No row calls a library-published `source=probe` figure, because none can
    exist here.
18. **The watchdog rule.** The lane reports the two printed figures and
    `kernel.nmi_watchdog`, and names the deduction a no-op on this part. It subtracts
    nothing by hand and writes no sysctl. It calls neither equality nor inequality a
    rule of the spec.
19. **No raw code.** The lane runs no tool that takes a type word and a config word, and
    publishes no raw config value. It names the device, the `type` file and the event
    file it read, and points at the standing record where a word would be needed.
20. **The golden.** The lane reports the device line as this host read it and cites the
    `.expected` file as another host's reading. It claims neither a match nor a failure
    of a byte compare.
21. **The permission row.** The record gives this host's `perf_event_paranoid` value,
    the device modes and the root status in the same row, and does not word a -1 reading
    as the equal of the standing 2 reading. Raising the value stands blocked, because it
    needs root.

**Part D.**

22. **The coverage chain.** The lane runs the CI coverage chain in order and records five
    exits: configure, build, the `-j 1` suite, the `coverage` target and
    `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` run by hand. The
    target exit is make's, and the gate exit is the verdict. Only 0 is green: 1 is a
    measured shortfall, 2 is a usage fault and 3 is a tool fault. The lane zeroes the
    counters before the suite, and names that step as a local addition.
23. **The threshold clause.** R-1, R-2 and W-1 resolve to the tracked set: the three lcov
    classes and the filter of `cmake/coverage.cmake`, the class the gate itself carries,
    the two `--omit-lines` patterns and the allowance of 21. The record says the three
    labels carry no definition of their own. It states four branch numbers, the host
    shortfall, the allowance, the base shortfall, and the claim that this branch adds no
    new shortfall. It claims no green from a host run against the union allowance, and
    it marks the function figure ungated.
24. **The plain suite.** The plain-suite rows use the CI `test` job form in `build`, a
    `ci-rocky` tree that carries the same cache variables as `ci-ubuntu-test`. Each row
    keeps `--no-tests=error`, prints the entry count from `ctest -N`, and names its
    parallelism, because parallelism decides verdicts on this host. The two-cpu row uses
    the full entry set under `taskset -c 0,1`, and the four-cpu row uses four allowed
    cpus and stands as extra evidence. The `prepush-quick` form drops entries and belongs
    to another tree, so it is not this row. No row adds `--timeout`.
25. **The sanitizer lanes.** Both lanes use CI's compiler, `CXX=clang++` at configure,
    and copy the sanitizer option strings out of the workflow. A run on the host default
    compiler is not the CI lane, and this branch reports none as one.
26. **A run that cannot finish.** The lane re-runs each cut entry alone in the same tree,
    names each extra command with its exit and elapsed time, and only then marks the row.
    No row widens a ceiling from the command line, and no row folds a per-file capture
    into the tree figure.
27. **A base failure.** A row may call a name a base failure when a tracked record names
    the same host, the same command form and the same entry. Any other red name gets a
    base run in a second tree, with the base sha, the tree, the command and the exit
    beside it.
28. **The analyzer.** A fresh tree carries the analyzer build, because a reconfigure of a
    built tree recompiles nothing and an empty finding list then proves nothing. The proof
    is three counts out of the log: translation units per scope, a non-zero count for
    every file the branch owns, and zero hits on the gate checks. The verdict follows CI's
    arithmetic, and the delta against a base reading decides whether a finding is new.
29. **The consumer-release rows.** The lane runs the whole consumer-release job: the two
    configures, the cache asserts, the archive symbol check, the ignore developer tree and
    the trap fixtures. The non-zero exits of the fixtures are the pass condition and stand
    inside the row.
30. **The DBC gates.** The lane copies the `dbc-gate` target's own exit and message, then
    runs that target's two commands verbatim with an interpreter that carries yaml, and
    names both interpreters and both yaml states. It keeps the argument list whole and
    prints the interface count the gates report.
31. **The prose gate.** The lane runs the tree form before the commit and the range form
    after it, and names the base sha of the range. A form that examined nothing gets no
    completion mark.
32. **A citation defect.** `results/gates.md` and `tasks.md` cite `SC-011`, and this
    spec's list ends at SC-010. No new record repeats that id.


#### Session 2026-11-07, round 2

One reader checked the 32 rulings against the tree, the workflow and the code, and
answered 27 confirmed, 4 that left a gap and 1 that was wrong. The owner ruled on the
five here, and on four readings the check brought up beside them. The round closed after
this pass: no item of round 1 stayed open, and no new item reached the bar.

33. **IA48-004 saves six values.** The requirement now names the six files of D-8 and
    makes `tracing_on` a reading of its own outside the saved set, and it asks for the
    mode as the word the quickstart helper prints, without brackets. Round 1 said
    "snapshot seven" and "normalize to the bracketed word", and both read against FR-019,
    D-8 and the helper at `quickstart.md:61-65`.
34. **The prose tree form carries its check word.** The tree form runs with
    `PROSE_CHECK=prose`; the default of the cmake wrapper is `all`, which also runs the
    commit check and prints a degenerate-range note while the head equals the base. The
    mark follows the prose unit count the gate prints, and a range that resolves to
    nothing gets no mark.
35. **The three labels have a home.** The exceptions stand in `specs/045-merge-fixes`
    FR-015, and the requirement now cites it. This spec's own plan uses R-1, R-2 and R-4
    for research findings, so the bare labels read two ways.
36. **The consumer-release exits are mixed.** `dbc_trap_fixture` answers non-zero,
    `dbc_test` answers 1, the `overrun` mode of `counters_trap_fixture` answers non-zero,
    and the nine other modes of that fixture answer 0. The row carries the exit each step
    expects, so no reader calls an expected exit a failure. Round 1 said all fixture
    exits stand non-zero, and that is wrong.
37. **The 041 tools and the raw word.** The lane runs the `counter_probe` example alone.
    It cites `specs/041-counter-probe/tools/group_probe.c` and `tools/probe_window.c` as
    the standing shape and names `group_probe` not run, because its first argument is a
    raw config word and its output prints one. This closes the clash between IA41-001,
    which points at the tools, and IA41-002, which bars a raw code.
38. **The deepest level has no field in the table.** The vendored Skylake-X json carries
    no level field, so the record takes the deepest level the mapper reaches from the
    standing audit point AP-9, which measured it with one ask per level, and quotes the
    block's own `level=1` line as the only level a run printed.
39. **The round trip names its inputs.** It closes with
    `./build/example/first_benchmark --compare=<a>,<b>` over the two method documents of
    this run and the `jitter:` rule, which loads both through `loadReport`. The record
    also carries the answer of a `hwlat:` rule over those documents, which is the status
    FR-028 names for a figure the record does not carry.
40. **The standing task rows stay.** `tasks.md` T054, T055 and T056 name the two records
    that do not exist. They stay as they stand, and the new record names them unrun.
41. **The `prepush-quick` reading is not run.** SC-003 asks for
    `taskset -c 0,1 ctest --preset prepush-quick -j 2`, and ruling 24 keeps that form out
    of Part D. The record names it not run rather than calling the masked full-suite row
    its equal.
42. **The analyzer file set stands empty.** The branch owns three record files and no
    translation unit, so the per-file proof of ruling 28 has an empty set here. The row
    carries the translation-unit count per scope and the gate-check hits, and says the
    owned-file set is empty.
43. **One host fact of the standing artifacts is stale.** `/proc/interrupts` answers its
    `NMI` row to an unprivileged reader on this host today, so the note that an
    unprivileged reader loses that row no longer holds at this check. The unprivileged
    method row still prints the SMI disclosure, because the `msr` node stays root-only.


## Acceptance closeout: measured Intel evidence (2026-11-08)

### Identity and binding

This section extends this spec, `specs/046-hunter-topdown` and
`specs/041-counter-probe` with one closeout phase. It adds no feature, and it
opens no new spec number. It keeps every id, every ruling and every record of the
2026-11-07 section above, and it measures again at the current head. Pull request
#98 measured `87d745d6c7e62d4da80198c689ddc7078f08ff3d`, which stands before the
fixture-macro data repair, before the capture suppression stood removed, and
before the direct latency predicate tests landed. Its branch, its cold-branch
count and its sanitizer counts stand here as leads to repeat. A lead becomes a
result only when a run on the current source answers it.

| Line | Value at the check on 2026-11-08 |
|---|---|
| Base head | `2d4d8c57975a86db6c77536f7090aba2bdaf6417`, and `origin/master` at the same head |
| Ancestry | `git merge-base --is-ancestor` answers the base an ancestor of `origin/master` |
| Branch | `fix/intel-acceptance-closeout`, cut from the latest `origin/master`; the branch stood absent on the remote at the check |
| Host | corsairone, Intel Core i9-9920X (Skylake-X), non-hybrid, one socket, 24 cpus online 0-23 |
| Version | 0.34.0, schema `1.16`, harness SOVERSION 22, archive SOVERSION 8 |
| Page and marker caps | README 1,002 lines against the cap of 1,004; 84 `LCOV_EXCL` markers against the cap of 84 |
| Open pull requests | none at the check |
| Kernel access | `perf_event_paranoid` -1, `nmi_watchdog` 0, `/sys/kernel/tracing` `drwx------ root`, `/dev/cpu/*/msr` `crw------- root` |
| Root rights | absent for this session: `sudo -n -l` asks for a password, and `pkexec --disable-internal-agent id` answers that no authentication agent is found |

The phase rechecks every line above at branch time and states each answer in the
record it writes.

### Changed files

The phase owns two kinds of file. Records and artifacts come first: the three
`results/acceptance-final-intel.md` files, the three `tasks.md` files, the three
`spec.md` files, the three `plan.md` files,
`specs/046-hunter-topdown/results/acceptance-hunt.md`,
`specs/046-hunter-topdown/results/pr-body.md`, and one new checklist under
`specs/048-hwlat-probe/checklists/`. No guide file stands in the list: `specs/048-hwlat-probe/quickstart.md` keeps its text, and a row of the record names the wording it diverges from. Test and driver
corrections come second, and only where a diagnosis names them:
`test/base_cli/run_matrix.py` with the matrix data and fixtures it reads,
`test/example_run.sh` with its fixtures, `test/source/harness_hunt_cli_test.cpp`,
and `test/source/harness_jitter_cli_test.cpp`. Each such change keeps the
acceptance contract of the entry it touches.

The phase changes nothing else. It edits no `test/CMakeLists.txt`, no timeout, no
barrier implementation, no barrier test, no FWQ/FTQ production, helper or unit
file, no `example/docs/fwq_ftq.cpp`, no `tools/docs/report_shape.py`, no fixture
or guide block of either, no shared coverage or DBC gate, no CMake preset, no CI
file, no public header, no schema, and no README or version file. Where a
diagnosis points at a file this list excludes, the phase records the exact
reproduction, defers that fix, and stops before the edit. A full acceptance row
that depends on a deferred fix stays open, and the report names it as a
follow-up.

### User Story 1 - A reviewer reads current Intel measurements (Priority: P1)

A reviewer opens the three records and finds, for the head the branch carries,
the counts a run answered on this host. The reviewer sees which rows a lead
supplied and a run later confirmed, and which rows a run refused.

**Why this priority**: the standing Intel record stands measured at a head two
repairs behind, and a relabeled old count reads as new evidence.

**Independent Test**: each row names the head it ran at, and a reader can rerun
the command in the row and compare the answer.

**Acceptance Scenarios**:

1. **Given** a record that carries a count from `87d745d6`, **When** the phase
   measures the same lane at the current head, **Then** the record keeps the old
   section as history and adds the new reading under its own dated heading.
2. **Given** a row no run answered, **When** a reader opens it, **Then** the row
   names the command, the missing right or the deferred cause, and carries no
   figure.

### User Story 2 - A slow check carries a verified cause (Priority: P1)

A maintainer opens a red instrumented lane and finds one cause per failed name,
each with the runs that separate it from the alternatives. A test that runs
because its contract needs the work looks different from a test that runs
because a fixture selects more work than its contract asks for.

**Why this priority**: four names stand red in the instrumented lanes, and a
retry that passes hides a defect that a repeat run would show.

**Independent Test**: the record names the original suite run, the isolated run,
and the bounded-concurrency run for each name, with the exit and the reason for
each.

**Acceptance Scenarios**:

1. **Given** an entry that times out in a lane, **When** the phase diagnoses it,
   **Then** the record gives the child command, the work it did, the exit, and
   both the script grant and the CTest ceiling that bound it.
2. **Given** a diagnosis that shows a fixture drives exhaustive hardware work
   inside a command-line contract test, **When** the phase corrects the fixture,
   **Then** the deterministic tests still prove the command-line contract, and
   the hardware claim moves to a row labeled as a hardware run.

### User Story 3 - A privileged row states its evidence (Priority: P2)

A reader wants to know whether a root run happened. The record either carries the
before and after values of each file the run touched, or it names the operator
action that stands missing.

**Why this priority**: the restore rows of SC-005 and SC-007 stand the part of the
detector surface no unprivileged session can reach.

**Independent Test**: each root row either carries two values per file from one
privileged run, or it states BLOCKED with the command an operator runs.

**Acceptance Scenarios**:

1. **Given** an operator-approved privileged session, **When** the run ends,
   **Then** the record compares each saved contract value one by one, both after
   a normal end and after the controlled interruption.
2. **Given** no privileged session, **When** the phase reports, **Then** every
   root row reads BLOCKED, names the exact operator action, and the unprivileged
   rows stay complete.

### Requirements - the closeout phase (IC-001 to IC-007)

- **IC-001**: The lane SHALL build the current source in clean, named trees for
  the release, coverage, address and undefined sanitizer, and thread sanitizer
  configurations. The record SHALL give the source sha, the cpu affinity the host
  allowed, the compiler launcher for each tree, the python interpreter each tree
  configured, the `perf_event_paranoid` and `nmi_watchdog` values, the reach of
  the `msr` node and of tracefs, and the exact command, exit code and entry count
  of the required suite. The lane SHALL preserve the original failing run before
  it runs any isolated diagnostic.
- **IC-002**: The lane SHALL complete the coverage target and the coverage gate on
  the measured source, with no inconsistent-data suppression in any command. For
  each cold branch the lane SHALL report what the raw capture, the captured trace
  and the extracted trace each hold. The record SHALL give the numerator and the
  denominator before the change and after it, the cold rows, and the gate exit.
  The lane SHALL add no ignore, omit, remove, exclusion or allowance, and SHALL
  call no partial union a pass. A cold-branch count from an earlier head stands
  recorded as that head's reading, and the lane SHALL NOT carry it forward as this
  head's answer.
- **IC-003**: For each sanitizer failure the lane SHALL record three runs: the
  original suite, one isolated run in the same configuration, and one run with the
  concurrency bounded to the hosted shape. For each run the record SHALL give the
  child command, the event or work count where the entry reports one, the exit
  code, the data or timeout reason, and both the grant inside the driving script
  and the ceiling CTest applies. A run that passes on a retry stands recorded as a
  diagnostic. A duration alone SHALL NOT clear a defect, and a hosted job SHALL
  NOT prove a race or a fault absent.
- **IC-004**: The lane SHALL correct a demonstrated defect only inside the owned
  scope, and each correction SHALL carry a regression test that fails before it.
  The correction SHALL preserve the behavior the entry tested. Where a fixture or
  a selection drives exhaustive real PMU work that the entry does not need, a
  narrow workload or selection correction stands allowed on two conditions:
  deterministic tests keep the command-line contract proved, and the hardware
  claim moves to a row that names the real hardware run it came from. The lane
  SHALL drop no assertion, SHALL skip no test, SHALL mask no failure, and SHALL
  replace no required work with a stand-in. The lane SHALL raise no timeout,
  SHALL raise no script grant, and SHALL add no retry.
- **IC-005**: The lane SHALL resolve the DBC gate target dependency through the
  ordinary setup of the host. The lane SHALL first read the interpreter the tree
  configured. It SHALL then give that interpreter its declared PyYAML dependency
  through approved means, or configure the tree with an interpreter the project
  already supports. The lane SHALL run the unchanged `dbc-gate` target and SHALL
  run the manual gate commands beside it, and SHALL report both. Where the setup
  needs a permission no session here holds, the row SHALL read BLOCKED with the
  exact operator action, and manual commands SHALL stand reported as manual. The
  lane SHALL bypass no gate, SHALL skip no hook, SHALL edit no CI file, and SHALL
  hardcode no personal interpreter path into any tracked file.
- **IC-006**: The lane SHALL correct and append the records under
  `specs/048-hwlat-probe`, `specs/046-hunter-topdown` and
  `specs/041-counter-probe`. Every tracked line SHALL give commands and outputs,
  and SHALL print a path as `$HOME`-relative or build-relative. Every blocked root
  row SHALL say so in words an operator can act on. The two hwlat refusals SHALL
  carry the error each one produces, so an invalid engine value reads differently
  from a window that stands missing. The parity rows SHALL state their reason:
  the historical requirement measured at `perf_event_paranoid` 2 describes a
  restricted environment, and an unreadable `msr` node on its own explains no
  refusal of ordinary perf group access. Old sections stay as they stand, and the
  record labels them history: a false line keeps its text and its heading, and the dated section carries the row that replaces it.
- **IC-007**: The lane SHALL run every required local command at the final
  squashed head, SHALL store the logs outside the repository, and SHALL carry the
  head sha, the commands, the counts, the root status, the output references and
  every failure in the pull request body. The phase is complete when the report
  names the base, the final head, the pull request, the root status, the hardware
  results, each diagnosed cause with its scoped fix or its deferral, the counts per
  lane, the record corrections, and the open blockers.

### Success Criteria - this extension

- **SC-IC-1**: Every required lane at the final head carries a command, an exit
  code and a count that a reader can reproduce, or names itself BLOCKED with the
  right it lacks.
- **SC-IC-2**: Each red instrumented name carries one diagnosed cause, the three
  runs that separate it from the alternatives, and either a scoped fix with its
  regression test or a deferral that names the excluded file.
- **SC-IC-3**: The coverage report gives the before and after numerators and
  denominators on the measured source, the cold rows the gate counts, and the gate
  exit, with no new suppression anywhere.
- **SC-IC-4**: The `dbc-gate` target answers exit 0 on the measured tree, or the
  row reads BLOCKED with the operator action and the manual readings stand
  labeled manual.
- **SC-IC-5**: Every row a closeout writes in the three records carries no user
  home path, no figure in a blocked row, and no verdict a run did not measure. A
  false line inside an older section keeps its text, keeps its heading, and gains a
  dated correction row that names the line it replaces.
- **SC-IC-6**: The pushed remote ref equals the local head, and the pull request
  body carries the final head sha with its own run counts.

### Constraints - this extension

- G-20: local runs decide acceptance. A hosted run informs a diagnosis, and a
  hosted pass stands recorded as a hosted pass. A base failure stays red until a
  fix clears it, and a host characteristic explains nothing on its own.
- G-25: no inconsistent-data suppression stands allowed in any coverage command,
  and no host lcovrc relaxation joins a gate.
- G-29: the two `group_io` closed-enum switch branch approvals stay the only
  marker approvals. The tree-wide branch allowance stays 21. No new exception,
  omit, ignore or exclusion enters any command, and the pull request #89 audit
  grants no gate permission.
- Thresholds stay as they stand: 100 percent lines and branches under the
  exceptions of `specs/045-merge-fixes` FR-015 with its R-1, R-2 and W-1, a branch
  shortfall of 21 rows at most, and at most 84 markers.
- A ceiling stands fixed. No timeout, no script grant and no concurrency setting
  moves to clear a required failure.
- Root rows need an operator-approved privileged session. The lane collects no
  password, automates no elevation, loads no module, adds no tracing group, edits
  no sudoers file, and weakens no tracefs, MSR or perf permission. Before an
  approved write the lane reads the active tracer and `tracing_on`, and stops the
  row when an unrelated session holds the tracer or a value reads unreadable. The
  lane normalizes a mode to the active word inside its brackets, and keeps
  `tracing_on` as a reading of its own.
- G-26 and G-27: old history, footers and authors stay. The new commit carries OmO
  as author and as committer. Title lines stand at 50 characters or fewer, body
  lines at 72 columns or fewer, in ASD-STE100 Simplified Technical English. The
  footer block carries, in order, `Refs: specs/048-hwlat-probe`,
  `Refs: specs/046-hunter-topdown`, `Refs: specs/041-counter-probe` and
  `Approved-by: OmO`, one trailer per line.
- G-23: the branch rebases onto the latest master, squashes to one commit, and
  never merges. Any tracked amendment or rebase makes a new head, and the lane
  squashes again and reruns the required commands on it. Evidence from an older
  head, or a claim that two heads look equivalent, stands outside final
  acceptance.
- G-31: this phase adds no raw event code support and publishes no raw
  configuration word. It reads G-31 as a boundary on new application support, and
  it leaves existing hardware test tools standing. N13e trace-line parsing and
  automatic rank selection stay outside this phase.
- The first push stands `git push -u origin fix/intel-acceptance-closeout`. A later
  push saves the full remote sha it last observed and uses
  `git push --force-with-lease=fix/intel-acceptance-closeout:<expected-full-sha>`.
  A failed lease stops the lane, and the lane refreshes no lease and retries no
  push. After each push the lane compares `git ls-remote origin
  refs/heads/fix/intel-acceptance-closeout` with `git rev-parse HEAD` and stops on
  a difference.
- The branch opens one pull request, merges nothing, and closes no issue. The
  `.specify/feature.json` pointer stays local and stays out of the commit.
- An unavailable SMI source, a zero count and a marked gap each say what they
  measured. None of the three proves the absence of live SMIs.

### Assumptions - this extension

- One session owns the working tree and the build trees for the length of the
  phase, and each instrumented run stands alone with a load witness beside it.
- Cpus 0 and 1 stand inside the affinity the host allows, so the two-cpu lane
  stands measurable here. The lane reads the affinity before it runs and marks
  itself BLOCKED where the answer says otherwise.
- A hardware benchmark row and a unit test row carry different evidence, and a
  real benchmark never stands in for a unit test.
- A root session, if an operator opens one, stands unattended by this phase: the
  phase reads values, runs one command, and writes nothing outside the contract of
  the run.
- The hosted runners keep their own cpu model, so a hosted count stands recorded
  with the job that answered it.


### Clarifications - this extension

#### Session 2026-11-08, round 1

Three readers checked this section against the tree, the workflow and the code.
Each named points where two careful executors would run different commands. The
owner ruled on all of them here. The readings a ruling retires stay named, so a
later reader can see the choice.

1. **A ceiling crossing is not yet a defect.** IC-004 asks for a demonstrated
   defect. A name that only crosses its ceiling stands recorded with its cause and
   stays red. A name crosses into the owned scope when a run shows it doing work
   its own contract does not ask for, and the count of that work stands in the
   record. Only then does a workload or selection correction stand allowed, and the
   deterministic tests named in the record keep the command-line contract proved.
2. **One directory per configuration.** "Clean, named trees" means the directory
   each preset names: `build`, `build/coverage`, `build/sanitize`, `build/tsan`.
   A lane wipes its own directory before it configures, and no two configurations
   share a directory. The release lane runs last of those four suites, because
   `build` is the parent of the other three, and the coverage trace stands copied
   to the external artifact directory before that wipe. `build/dev` and
   `build/analyzer` configure after that wipe, so no gate row reads a tree the
   release lane deleted. One row re-configures `build` with the audit preset,
   `IA48-056`, and that re-configure is the last thing the tree does: it runs
   after every figure that needs the release tree, and the record states that the
   release tree does not survive it. `IA48-061` carries that record line.
3. **The target and the gate carry separate exits.** The lane runs
   `cmake --build build/coverage -t coverage` unchanged and reports its exit. It
   runs `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` unchanged and
   reports that exit beside it. A target that stops at its HTML step names the
   module and the operator action, and reads BLOCKED for that step. The coverage
   gate row keeps its own verdict from its own run. No flag enters either command.
4. **Four words, one meaning each.** A row reads PASS when a run answered it, FAIL
   when a run answered and the contract did not hold, and BLOCKED when no run in
   scope could answer because a right, a tool or an operator action stands missing.
   A row that cannot close because its fix sits in an excluded file reads open with
   a named follow-up, and it carries its measured figures beside that name. The rule
   that a blocked row carries no figure reaches only the BLOCKED class.
5. **Before and after name heads.** The primary pair compares the capture taken at
   the base head against the capture taken at the final head, both in one shape:
   the serial suite, then the hand re-runs of every entry a ceiling cut. Where a
   lane also holds the serial-only capture from before its hand re-runs, the record
   reports that reading too, because it prices each ceiling. No lane runs twice for
   a number.
6. **Three runs per failed name, and one hosted shape.** Every name that fails in
   either instrumented lane gets the original suite run, one isolated run in the same
   tree with the same compiler, the same sanitizer options and the same CTest
   ceiling, and one run bounded to `taskset -c 0,1,2,3` with `-j 4`, the width a
   hosted runner answers. The record names each shape it ran.
7. **Suppression means the inconsistent class.** The lane changes no coverage
   command. It reports the tracked command verbatim, including the error classes the
   capture step names today. A stripped variant stands only as a labeled diagnostic
   and never as an acceptance run, so a tool complaint at exit 3 cannot become a
   pass and cannot become a suppressed pass either.
8. **Owned means the file list.** A shared fixture header that seven test sources
   include stands outside the owned scope, because the list names files and that
   header belongs to more than one entry. Eight registered CTest entries run a
   binary that includes it: six of the `harness_hunt_*` binaries directly,
   `harness_hunt_json_test_no_hunt` through the shared json binary
   (`test/CMakeLists.txt:1869`), and `harness_hunt_rank_trap_test` by running
   `harness_hunt_rank_trap_fixture` as its child (`test/CMakeLists.txt:1817-1821`). A diagnosis that lands there records the exact
   reproduction and defers the change.
9. **The hunt claim about a live catalog needs one run.** The standing sanitizer
   note blames `harness_hunt_cli_test` on a catalog of 497 live events, while that
   file and its fixture state a scripted catalog of 200 and no PMU read. One
   scenario run through the built binary settles it, and the record prints the
   `hunt: plan events=` line the run answers. A diagnosis inherits no claim from a
   record.
10. **The record labels each head it measured, and the reruns are not carries.** A
    figure from `87d745d6` keeps that head as its label and stands history. A lane
    that wants a figure at the closeout head runs the lane again. The two never share
    a row.

#### Session 2026-11-08, round 1 (continued): hardware and records

11. **Parity gets two rows.** One row stands measured at the host's standing
    `perf_event_paranoid` value of `-1`: `counter limit: cpu limit=4 source=decode
    probe=4` with exit `0`. One row stands BLOCKED for the paranoid `2` reading that
    SC-010 asks for, and names the operator action and the value to restore. Neither
    row borrows the other's figure.
12. **Three consoles per command, normal method only.** IH-001 asks three runs per
    command, so a closeout answers with three normal-method consoles per command,
    each alone on an idle host. A run through an alternative console is supplementary
    and never counts toward the three, because SC-007 binds its three runs to the
    normal method.
13. **Opens and reads: one measured number, one derived number, no new tool.** The
    library answers the group-size question with no device file open at all, because
    its decoder answers first. The example's own search opens up to 15 device files
    and reads up to 5 before it answers. The record prints both numbers and marks
    which one a run measures and which one the standing proof tool's arithmetic
    derives. No acceptance row runs a tracer.
14. **The probe tool surface stays `counter_probe`.** IP-002 bars no existing
    hardware test tool, and this closeout adds none, so the row names the one tool
    the standing record runs. `probe_window` takes no raw event word and answers no
    row here. The record names it and leaves it out.
15. **Rank movement gets both denominators, per command.** Each command reports
    movement among the 20 ranked rows and movement across all 497 printed rows, on
    its own three consoles. A comparison between the two binaries gets its own line
    and its own label, so no reader mixes a within-binary spread with a cross-binary
    one.
16. **A criterion id comes from the spec that owns the claim.** Where an `SC-` id
    exists, the record cites it. Where a device-class claim has no `SC-` id, the
    record cites the audit point or ruling that owns it and names that clause in the
    row, so the row never cites a criterion that does not exist.
17. **Every measurement lane runs alone.** The duty to run an instrumented suite
    alone on an idle host reaches the hardware rows too, and the record prints a load
    witness with each. A figure taken while another lane runs stands void and gets
    re-taken.
18. **The digest block prints verbatim.** IH-001 prints the full `table=` digest and
    the full `reference=` digest for each of the 13 top-down rows, at their length
    from the binary, with the level line beside them. The standing record's
    sixteen-hex form stands as history and does not carry.

#### Session 2026-11-08, round 1 (continued): records, closing, and the commit

19. **The closeout claims only what it ran.** IC-007 asks every named command to run
    at the final head and each row to carry its own verdict. Where a lane stays red,
    the closeout names the cause, the file that holds it, and the action that would
    close it, and the pull request says in plain words that the lane stands open. A
    red lane never closes a phase by silence, and a phase never calls a lane green
    because the rest passed.
20. **The final-head set is the whole closeout set, in clean trees.** The commands a
    closeout repeats at the final head are: the release suite in a clean `build`, the
    `cpu0,1` and `cpu0-3` release runs, both instrumented suites in their own clean
    trees, the coverage chain and its gate, and every gate command the workflow
    carries, including the `dbc-gate` target. A tree built at an earlier head answers
    no final-head row.
21. **Before prices the change, and the within-head pair prices a ceiling.** The
    before figure comes from the base head, the after figure from the final head, and
    both readings state the head, the command and the tree. Where a lane also holds
    the capture taken before its own hand re-runs, that pair stands reported as the
    price of each ceiling, under its own label.
22. **The `dbc-gate` pass route names the interpreter, and installs nothing.** A
    package manager refuses both interpreters on this host, so the route a closeout
    takes is the supported interpreter configuration, with the unchanged target and
    the unchanged manual commands run side by side. A row that shows the target and
    the manual commands agreeing at exit `0` closes the criterion.
23. **A claim a tracked file disproves is not a defect.** A research pass read two
    matrix rows as broken, on the reading that each side stops at a usage fault and
    so compares two failures. The tracked data disproves it: the rows stand at
    `test/data/base_cli_matrix.txt:10` and `test/data/base_cli_matrix.txt:21` with
    the expected status `2` and the exact fault line each one asks for, and no row of
    that file names `--interval` or `-r`. Row `case10` stands at
    `test/data/base_cli_matrix.txt:10` and row `case21` at
    `test/data/base_cli_matrix.txt:21`. IC-004 therefore names no defect in the
    matrix. It keeps the shared cause named in ruling 9's family of evidence: a
    provider walk that every child process pays and the fixture then discards, with
    its fix sitting in an excluded file, so each affected row stays open with that
    follow-up instead of closing on a retry or on a ceiling that moved.
24. **Residue gets six rows and one row without figures.** The record shows six
    before-and-after rows, one per saved file, and a seventh row for the threshold
    residue that carries no figure, because a residue has no number to print. The
    operator block answers with two privileged runs: one that prints the six values
    as they stand, and one that prints them again after the harness restore path.
25. **A planted reading closes its own row and no more.** The fixture tests close the
    planted-value row for the SMI counter. A reading from the real counter on this
    host needs the root-only device file, stands BLOCKED, and cites the older host's
    record as history. That history file stays as written.
26. **The commit carries one section token, three `Refs:` lines and one
    `Approved-by:` line.** The title reads `Acceptance: Intel closeout records and
    matrix rows`. OmO stands as both author and committer. The closeout names no
    issue, so the commit carries no `Fixes:` trailer, and the three `Refs:` lines
    name the three specs in the order `048`, `046`, `041`.
27. **History keeps its paths, and a new row does not.** Three home-directory paths
    and quoted `/tmp` commands stand inside record sections a closeout must not
    rewrite, so they stay and each one keeps its heading. Every row a closeout writes
    uses a `$HOME`-relative form, and the corrected sections carry no such path.

28. **A repeat of a count is not a move.** Two hosted runs of identical source at
    pull request #98 answered 27 uncovered branch rows and then 23 rows over the same
    9,823 branch positions, so the count of cold rows carries a spread of its own. A
    closeout count within a couple of rows of the standing figure reads as a normal
    repeat of it, and the row names its run, its tree and its head. A patch that
    changes no measured file claims no credit for a count that moved, in either
    direction.

## Intel acceptance closeout at the merged head (2026-11-08)

`a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a` stands the base of this phase, and
`origin/master` answers that head at the check on 2026-11-08. The base holds pull
request #99, whose merge commit
`6fba993858a2fff97f2e0c617152019bf7fe20fd` records the coverage gate closing at
the measured head `2d4d8c57975a86db6c77536f7090aba2bdaf6417`. The base also holds
pull request #100, and its merge commit is the base itself. That change moves ten
paths: `source/harness/detail/internal.hpp`,
`source/harness/detail/jitter_probe.hpp`,
`source/harness/execution_context.cpp`, `source/harness/jitter_probe.cpp`,
`test/source/harness_context_barrier_test.cpp`,
`test/source/harness_fwq_ftq_test.cpp`,
`specs/045-merge-fixes/spec.md`, `specs/045-merge-fixes/plan.md`,
`specs/045-merge-fixes/tasks.md` and
`specs/045-merge-fixes/results/barrier-fwq-closeout.md`. `gh pr view 100 --json
files` answers that list. The wait arms of
`source/harness/execution_context.cpp` and the spectrum hold of
`source/harness/detail/jitter_probe.hpp` are the two harness changes that bear on
this phase. The section above, `Acceptance closeout:
measured Intel evidence (2026-11-08)`, stays as history with every id, every
ruling and every record inside it. Its measured head `2d4d8c57` and the branch's
first closeout commit `92bacc03`, which the rebase onto this base replaced, keep
their own labels, and no row of this phase moves a figure from either head onto a
row of its own.

This phase re-measures on that base, and it reopens no barrier or FWQ source file.
It names three heads. The base head is `a55728af`. The pre-rebase head is
`92bacc03`, the closeout commit the rebase replaced. The final head is the head
Pass 2 runs at, and ruling 29 of `Rulings - this phase (29 to 42)` in this file
sets its shape: the squash after the Pass 1 writes produces that head and every
figure names it, and the squash after the Pass 2 writes produces the head the
pull request carries. Both squashes stand unmade at the check, so each reads no
figure. Every row that
names either head reads no figure for that sha until a record prints it, and
each record then prints the sha beside the phrase.

The ids keep the standing text above, and seven readings change: `IC-001` adds
the compiler and widens the suite to the final-head set, `IC-002` names the two
traces, `IC-003` leaves the duration bar out, `IC-004` names five reds,
`IC-005` keeps one interpreter route, `IC-006` leaves the refusal and parity
out, and `IC-007` leaves the body contents out. Three numbered
ruling lists live in this file: the 2026-11-07 list under `Clarifications` of
the `Acceptance extension: Intel final acceptance (2026-11-07)` section, the
2026-11-08 closeout list under `Clarifications - this extension`, and this
phase's list under `Rulings - this phase (29 to 42)`. A ruling cite names the
file, the list name, and the ruling number. That rule governs a cite that
crosses a file boundary, the place where a bare number becomes ambiguous.
Inside the file that holds a list, the list name alone settles the cite, and
each list above lives in one file.
One base-carried cite needs its handle named once, because an in-place edit
would rewrite a base line: the `Roadmap position` section of this file quotes
`specs/027-jitter-probe/spec.md:335` and `:901` for the deferred N13d entry.
The handle reads: the `Roadmap position` sentence of
`specs/027-jitter-probe/spec.md` that names N13d, and the `Out of scope`
sentence under its `Scope` heading that names the same entry. The quoted
numbers keep their text as the reading of the check that wrote them, and the
handle is the pointer a run follows.

### Requirements - this phase (IC-001 to IC-007 restated)

- **IC-001**: The lane SHALL build this base in clean, named trees for the
  release, coverage, address and undefined sanitizer, and thread sanitizer
  configurations. The record SHALL give the source sha, the cpu affinity the host
  allowed, the compiler and the compiler launcher of each tree, the python
  interpreter each tree configured, the `perf_event_paranoid` and `nmi_watchdog`
  values, the reach of the `msr` node and of tracefs, and the command, exit code
  and entry count of each suite of the final-head set. The lane SHALL preserve
  each original failing run before it runs any isolated diagnostic.
  `specs/048-hwlat-probe/results/acceptance-final-intel.md` holds these rows.
- **IC-002**: The lane SHALL run the coverage target and the coverage gate on
  the measured source, and each SHALL answer with its own exit. The capture
  step and the summary step SHALL complete. The picture step SHALL read BLOCKED
  with the operator action `pacman -S perl-gd gd`, and the row SHALL state the
  target exited 2 there beside the gate's own exit. The gate reads that
  capture's text summary, so the blocked step blocks no gate row. No command
  SHALL gain inconsistent-data suppression. The build writes two traces,
  `build/coverage/coverage.raw.info` and `build/coverage/coverage.info`, and
  the lane SHALL report what each holds for every cold row. The gate's printed
  list serves a failing run, and the trace is the source for a passing run. The
  record SHALL give the numerator, the denominator, the cold rows and the gate
  exit at `a55728af` and at the final head. The lane SHALL add no ignore, omit,
  remove, exclusion or allowance, and SHALL call no partial union a pass.
  `specs/048-hwlat-probe/results/acceptance-final-intel.md` holds these rows.
- **IC-003**: For each sanitizer failure this base shows the lane SHALL record
  three runs: the original suite, one isolated run in the same tree with the same
  compiler, the same sanitizer options and the same CTest ceiling, and one run
  bounded to `taskset -c 0,1,2,3` with `-j 4`. The narrow shape stands beside it:
  `taskset -c 0,1` with `-j 2`. A hosted suite run uses the width the workflow
  asks for, and the lane prints that width. For each run the record SHALL give the
  child command, the event or work count where the entry reports one, the exit
  code, the data or timeout reason, the grant inside the driving script and the
  ceiling CTest applies. A run that passes on a retry stands recorded as a
  diagnostic.
  `specs/048-hwlat-probe/results/acceptance-final-intel.md` holds these rows.
- **IC-004**: The lane SHALL change no code unless a run finds a demonstrated
  defect inside the owned file list. Where such a defect stands found, the
  correction SHALL carry a regression case that fails before it, passes after
  it, and preserves the behavior tested, and that regression case lands only
  then. This ruling states the empty set of code writers as the shape of this
  phase: a `.cpp`, a header, a script or a data file changes only where such a
  run answers, and no run has answered one yet. A workload or selection
  correction stands allowed on the two standing conditions. The record carries
  the five red names the two instrumented lanes printed at `2d4d8c57`. Two name
  a file inside the owned list: `harness_jitter_cli_test`, which selects its own
  work in an owned file, and `base_cli_matrix`, whose runner
  `test/base_cli/run_matrix.py` stands inside that list while the cause of its
  ceiling crossing stands outside it, so that row reads open with its
  reproduction. Three name a file outside it and read open with their
  third-shape runs and reproduction: `harness_hunt_cli_test` with its shared
  fixture header, `example_run` with the example programs, and
  `report_shape_fwq_ftq_sample` with its driver and its registration. The lane
  SHALL drop no assertion, SHALL skip no test, SHALL mask no failure, SHALL
  raise no timeout, and SHALL add no retry. Each run stands in
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`.
- **IC-005**: The lane SHALL resolve the `dbc-gate` target dependency through the
  ordinary setup of the host. The lane SHALL read the interpreter the tree
  configured, SHALL verify that interpreter's declared PyYAML dependency, and
  SHALL run the unchanged target beside the unchanged manual commands. The package
  manager here refuses both interpreters, so the lane SHALL configure the tree
  with an interpreter the project already supports. Where the setup needs a
  permission no session here holds, the row SHALL read BLOCKED with the exact
  operator action, and the manual readings stand labeled manual. The lane SHALL
  bypass no gate, SHALL skip no hook, SHALL edit no CI file, and SHALL hardcode no
  personal interpreter path in any tracked file.
  `specs/048-hwlat-probe/results/acceptance-final-intel.md` holds these rows.
- **IC-006**: The lane SHALL append and correct the records under
  `specs/048-hwlat-probe`, `specs/046-hunter-topdown` and
  `specs/041-counter-probe`. Every line the phase writes SHALL print a path as
  `$HOME`-relative or build-relative, SHALL carry its command and its output, and
  SHALL leave every blocked root row with the action an operator can take. Every
  code anchor inside a dated row SHALL be re-read at the measured head, because
  pull request #100 moved two anchors this branch already cites, in
  `source/harness/detail/internal.hpp` and `source/harness/jitter_probe.cpp`. The
  record SHALL keep pull request #99 attributed to the head that run measured, and
  an older line keeps its text and its heading while a dated row names the line it
  replaces. The three `results/acceptance-final-intel.md` files hold these rows.
- **IC-007**: The lane SHALL run the final-head set at the final head in clean
  trees, SHALL store every log outside the repository, and SHALL print that head
  sha in each record and in the pull request body. The lane SHALL name both heads
  of ruling 29 of `Rulings - this phase (29 to 42)` in every figure row, and
  SHALL print that ruling's markdown-only proof clause there. A delta that
  changes markdown files only leaves every figure in place, so this requirement
  holds at either head. The rule that bars a user home path bars a path under the
  operator's home directory, and a run's external log directory under `/tmp` is
  not such a path, so a record writes a log path as the run printed it. The
  closeout body is appended to
  `specs/046-hunter-topdown/results/pr-body.md` under its own dated heading, and
  the standing body of that file keeps its text and its heading.
  `specs/048-hwlat-probe/results/acceptance-final-intel.md` holds the set.

### The final-head set

The set has ten lanes:

- the coverage chain, with its four hand re-runs, its capture and its gate;
- the address and undefined sanitizer suite in `build/sanitize`;
- the thread sanitizer suite in `build/tsan`;
- the release suite in `build`;
- the release run bounded to cpus 0 and 1;
- the release run bounded to cpus 0 to 3;
- the walk pricing;
- the developer-mode gate tree, its `dbc-gate` contract target and the two prose
  gates;
- the hardware consoles;
- the static-analysis tree in `build/analyzer`.

`specs/048-hwlat-probe/plan.md` carries the command of each lane in its lane table
and in the command blocks under that table.

### Files this phase may change

The list holds seven entries:

- `test/base_cli/run_matrix.py`, its matrix data and its fixtures;
- `test/example_run.sh` and its fixtures;
- `test/source/harness_hunt_cli_test.cpp`;
- `test/source/harness_jitter_cli_test.cpp`;
- the markdown artifacts of `specs/048-hwlat-probe`: the specification, the plan, the tasks, the checklists and the results;
- the markdown artifacts of `specs/046-hunter-topdown`: the specification, the plan, the tasks, the checklists and the results;
- the markdown artifacts of `specs/041-counter-probe`: the specification, the plan, the tasks, the checklists and the results.

`specs/048-hwlat-probe/results/gates.md` stands outside the list, and this phase
cites its contract-gate rows by reference and rewrites none of them.

### Success criteria - this phase (SC-IC-7 to SC-IC-11)

- **SC-IC-7**: Every lane of the final-head set carries at the final head a
  command, an exit code and a count a reader can rerun, or reads BLOCKED with the
  right it lacks. The Pass 2 set of ruling 29 of `Rulings - this phase (29 to 42)`
  in this file answers this criterion, and no row quotes a Pass 1 run. Every
  figure row names both heads of that ruling and prints its markdown-only proof
  clause, so this criterion holds across a delta that changes markdown files
  only: the head Pass 2 ran at answers it, and the head the pull request carries
  answers it beside that clause.
- **SC-IC-8**: The coverage gate answers within the allowance of 21 rows on the
  denominator its own capture prints, and the record names that denominator
  beside the shortfall. The run that answers this criterion is
  `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` at the final
  head.
- **SC-IC-9**: Each instrumented lane runs its suite twice at one head in one
  tree and repeats its passed and failed counts. An instrumented lane is the
  address and undefined sanitizer tree or the thread sanitizer tree. The row
  prints both exit codes and both times. The criterion passes when the two runs
  report the same set of red names. A differing set fails the criterion. The
  second run answers beside the first, and no earlier-head count answers it.
  The two rules govern different things, ruling 5 of the 2026-11-08 closeout
  list in this file a number and this criterion a set of red names, so a repeat
  that prints a set is not a repeat that produces a number.
- **SC-IC-10**: Each of the five red names `IC-004` names carries its cause
  inside the owned file list, or a deferred reproduction naming a file outside
  it. The three runs of `IC-003` answer this criterion at the final head.
- **SC-IC-11**: The push answer matches the local head. `git ls-remote origin
  refs/heads/fix/intel-acceptance-closeout`, read after the push, answers this
  criterion against `git rev-parse HEAD`.

Four figures this phase needs stand unanswered at the check on 2026-11-08. Each
row gives the command, says no figure, and names the run that must answer it.
Every figure row of this phase names both heads of ruling 29 of
`Rulings - this phase (29 to 42)` in this file and prints that ruling's
markdown-only proof clause. `IA48-059` carries the record rows that do so.

| Reading | Command | Answer at the check |
|---|---|---|
| Entries the release tree registers | `ctest --test-dir build -N` | no figure; the lane reads the last line of that answer, and the Pass 2 release tree at the final head answers it |
| Lines the capture covers | `cmake --build build/coverage -t coverage` | no figure; the Pass 2 coverage chain at the final head answers it |
| Branch shortfall and its denominator | `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` | no figure; the Pass 2 gate run at the final head answers it |
| Cold rows of the extracted trace, and what each trace the build writes holds there: a branch row of `build/coverage/coverage.info` where at least one outcome counts zero | `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` | no figure; the Pass 2 gate run at the final head answers it with the cold rows it reads there, and the gate prints its own row list only on a failing run, so the trace is the source for a passing run |

### Rulings - this phase (29 to 42)

29. **Two passes, and the second one owns the record.** A row cannot quote a run
    made before its own head existed. Pass 1 is the whole final-head set run at a
    candidate head, so that its figures can be written into the records. The
    squash after those writes produces the head Pass 2 runs at and every figure
    names. The squash after the Pass 2 writes produces the head the pull request
    carries. Pass 2 is the same set run at the head Pass 2 runs at, after the
    Pass 1 records stand committed. Pass 1 may guide edits. Any tracked change
    after Pass 1 forces a new squash and a fresh Pass 2 at that head. Every
    figure row names both heads, and each prints the markdown-only proof clause:
    `git diff --name-only <head Pass 2 ran at> <head the pull request carries>`
    prints files under `specs/` alone, so the delta between the two heads holds
    markdown files only and no figure moves. A diff that names any other path voids the
    figures and forces a fresh Pass 2.
30. **A figure is void under another sweep's load.** A figure stands only when
    that lane's own witness line reports no other compiler, test, capture or trace
    process, and a one-minute load average below 2.0. Otherwise the figure is
    void, and the lane re-takes it alone with its load witness beside it.
31. **A matching count pair does not settle a moving count.** The repeat rule and
    its spread stand in ruling 28 of the 2026-11-08 closeout list in this file,
    and a count within two rows of the standing figure reads as a repeat of it.
    The local coverage denominator and the hosted coverage denominator are two
    different totals, and no row compares them.
32. **The picture step stands outside the gate.** The coverage target may stop at
    its HTML step, and the gate keeps reading the text summary of the same
    capture. That step names its operator action, and the gate row keeps its own
    verdict.
33. **An isolated run that passes is a diagnostic.** It records a condition, and
    it closes no defect. A correction stands only where a run shows work the entry
    contract does not ask for.
34. **Two passes per cut entry.** An entry a ceiling cut gets its own command
    with that ceiling, then its own registration run direct: `ctest --test-dir
    <tree> --show-only=json-v1 -R '^<name>$'` prints the program, its arguments
    and its working directory, and running that program there carries no CTest
    limit. Both runs stand in one tree at one head, and the record prints both
    counts. A grant an entry enforces inside its driver still binds
    (`tools/docs/report_shape.py:368`).
35. **Each figure names the head whose run answered it.** A figure taken at
    `2d4d8c57` keeps that label, a figure taken at the pre-rebase head `92bacc03`
    keeps that label, and a final-head row carries a final-head run.
36. **The owned file list is the only place a code line changes.** A diagnosis
    that lands outside it records the exact reproduction, names the file, and
    defers the change.
37. **Root rows stay BLOCKED without a privilege.** Each blocked root row names the
    exact operator action and the value to restore. The phase collects no password,
    automates no elevation, and weakens no kernel control.
38. **The push carries a lease, and the remote answers.** No remote branch of
    this name exists yet, so the first push runs
    `git push -u origin fix/intel-acceptance-closeout`. A later push runs `git
    push --force-with-lease=fix/intel-acceptance-closeout:<expected-full-sha>`.
    Every push stands followed by `git ls-remote origin
    refs/heads/fix/intel-acceptance-closeout`, and the sha of that answer must
    equal the local head.
39. **The pull request body carries the set.** The body names the final head sha,
    the count per lane, the root status, and every deferred cause, in lines of 72
    columns or fewer.
40. **One sha closes the phase.** The record, the pull request body and the remote
    branch must agree on one head sha. Any disagreement reads open, and no part of
    the set closes the phase alone.
41. **The digest rule reads as the code holds it.** Ruling 18 of the 2026-11-08
    closeout list in this file takes the reading the code gives.
    `source/harness/report.cpp:367-370` prints the level line with its
    `passes=` count and a `table=` directory path, `:378-380` prints the
    reference line with the manifest reference where the record carries one,
    and `:381-384` prints one `topdown: file` digest line per file the record
    carries. Neither the `table=` field nor the `reference=` field carries a
    digest. The count of digest lines is a reading of the run, because the
    binary prints one line per contributing file, so no row pins it. A row that
    quoted 13 digest lines keeps its text and keeps its heading, and gains a
    dated correction row in its record naming the line it replaces, the duty
    `SC-IC-5` of `Success Criteria - this extension` in this file states.
42. **A blocked row cites the identity reading.** `SC-IC-5` of
    `Success Criteria - this extension` in this file bars
    a figure in a blocked row. The value a host answered for a kernel control
    before an operator changes it belongs to the identity reading of that run,
    and it figures no blocked measurement. The blocked row SHALL cite the
    identity section of the same run for that value, SHALL print the operator
    line and the restore line, and the operator reads the value before the
    change.

### Out of scope for this phase

- The barrier and FWQ implementation files, their private headers, the barrier
  fixture and the FWQ unit file, because pull request #100 landed them.
- `test/CMakeLists.txt`, the shared coverage and DBC gates, every CMake preset and
  every CI file.
- The allowance and its ledger in `tools/dbc/coverage_gate.sh`, and the branch
  position count that `cmake/coverage.cmake:111-125` produces. The hosted
  shortfall stands deferred with its reproduction and no figure, and one local
  pass answers nothing about the hosted red.
- The schema, the version files, and `README.md`.
- The files of another host and the files of another lane's branch.
- Raw event code support and any published raw configuration word, which G-31
  leaves as future work.
- Any weakening of a kernel control: `perf_event_paranoid`, `nmi_watchdog`, the
  tracefs mode and the `msr` mode stay as the host holds them.
- Any timeout, script grant, ceiling, retry, coverage marker, allowance or
  suppression.

### Assumptions

- The host keeps the rights it holds today, and an operator change to them stands
  recorded when a run reads it.
- `perl-GD` stays absent, so the text summary stays the source the coverage gate
  reads and the picture step stays a named step.
- The analyzer lane may use a tree of its own, because that tree answers no row of
  another lane.
- A hosted failure at master stands disclosed in the record, and this phase
  repairs nothing inside a container.
- `.specify/feature.json` is the local pointer of the spec-driven tooling. It is
  tracked, this phase sets it while it works and restores the value the base head
  carries before any commit, so it stands outside `Files this phase may change`.
  `IA48-058` is the one row that owns that file, and its verify line prints
  `git status --porcelain .specify/feature.json` empty before each commit. The
  file stands modified at the check on 2026-11-09: the tooling holds the feature
  directory in use, this phase works in `specs/048-hwlat-probe`, and the base
  head carries `specs/045-merge-fixes`. That row records the standing
  modification with that cause.
