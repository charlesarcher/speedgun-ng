# Research: Hardware Latency Probe with SMI Attribution

Phase 0 output of `/speckit.plan` for `specs/048-hwlat-probe/spec.md`. Each
entry names the choice the code makes, the reading that proves it, and the
losing alternative. Every cite reads the working tree at the base `858c55d9`
(version 0.31.0, schema 1.14, harness `SOVERSION` 21, archive `SOVERSION`
6), the base of branch `048-hwlat-probe`. A cite that names `kernel/trace/`
reads the upstream copy of that file. The lane host is an Intel Skylake-X
i9-9920X on one socket, kernel `7.3.0-4-cachyos`, GCC 16.2.1, read as an
unprivileged user except where a line names its rights.

Numbering: the entries here read `R-1` to `R-16`, the spec's decisions read
`D-1` to `D-15`, the plan's rulings read `P-1` to `P-14`, and a requirement
reads `FR-0NN`. A bare `:NNN` continues the file of the cite above it.

## R-1: the file root as the one door

- **Decision**: Every path this lane opens resolves through `procRootPath`
  (`source/detail/proc_root.hpp:35`), and every test installs a fake tree
  through `installTestProcRoot` (`:52`). One new unit,
  `source/harness/latency_counters.cpp`, opens the `/proc` and `/dev/cpu`
  sources, and the tracer unit keeps the tracefs opens.
- **Rationale**: The seam joins an installed root or `/`, so production
  names the absolute path and a test names its scratch tree
  (`source/detail/proc_root.hpp:35-41`, `:52-56`). The tracefs reads already
  resolve through it (`source/harness/noise_tracer.cpp:320`, `:324`,
  `:367`), and the pre-check reads `proc/interrupts` through the same door
  at `source/harness/jitter_probe.cpp:2055`, so the counter unit reaches
  `/dev/cpu` the same way (spec FR-001, plan P-1).
- **Alternatives considered**: A second seam for the register nodes: the
  plan's Constitution Check X bars a second seam, and a test would then
  install two roots. An absolute open of `/dev/cpu/0/msr`: no test reaches a
  root-only read, and the suite would fail on a host without the nodes (spec
  FR-032).

## R-2: the register read at offset `0x34`

- **Decision**: The unit opens `/dev/cpu/<cpu>/msr` once at probe start with
  `::open`, reads 8 bytes at offset `0x34` with `::pread` into a stack
  buffer, and reads them as one little-endian unsigned 64-bit count. It
  never writes the node and it loads no module.
- **Rationale**: Spec FR-007 fixes the address, the width, the byte order
  and the no-write rule, and FR-008 bars a module load. Plan P-2 fixes the
  shape: one open, one `pread` per read, a stack buffer, and the close at
  stop, so the gap path allocates nothing. The node is root-only: on the
  lane host `/dev/cpu/0/msr` stands present and answers `EACCES` to an
  unprivileged reader (spec A-2), so `not available` stands the ordinary
  path of this feature (spec FR-011).
- **Alternatives considered**: `std::ifstream` with a seek: it allocates,
  and plan P-2 keeps the text streams off the gap path. The perf `msr`
  device behind `smi_num`
  (`external/pmu-events/arch/x86/alderlake/adl-metrics.json:60`) needs
  `perf_event_open`, and no harness test opens it.

## R-3: the vendor gate reads a text line

- **Decision**: The gate reads the `vendor_id` line of `/proc/cpuinfo` under
  the seam. `GenuineIntel` supports `MSR_SMI_COUNT`, and any other text or
  an unreadable file answers unsupported.
- **Rationale**: Spec D-4 rules that a part with no documented counter
  answers not available, because the counter at `0x34` stands undocumented
  across the AMD families, and FR-010 fixes the reading of the text so a
  test plants the line it needs. The alternative stands built and
  unreachable: the counters library reads a CPUID-based `PmuIdent` that
  carries vendor, family, model and stepping
  (`source/counters/detail/pmu.hpp:85-95`, `pmuIdentCurrent` at `:106-107`)
  inside a header no harness file includes.
- **Alternatives considered**: A `CPUID` read: it needs the instruction, no
  fake answers an instruction, and spec FR-032 bars a test that reaches the
  host, so no case could plant a part. A model table: it names parts the
  lane cannot cite, which spec D-4 refuses.

## R-4: the `NMI` row beside the standing sum

- **Decision**: A new reader takes one cpu's column from the `NMI:` row of
  `/proc/interrupts`. The standing `interruptsOfText` keeps its sum over
  every labeled row, so the new figure stands beside `interrupts` under its
  own name.
- **Rationale**: `interruptsOfText`
  (`source/harness/jitter_probe.cpp:1811-1839`) sums one column over every
  line that carries a colon and drops the label, so an `NMI` count already
  rides the standing figure under no name (spec FR-009). The file reads
  through `readUnderRoot` at `:1778-1791`, the call at `:2055`, so the row
  costs no second path. An unprivileged reader here sees 83 lines and no
  `NMI` row, because the kernel withholds the `NMI` and `LOC` rows (spec
  A-5), so the disclosure path of FR-011 stands the ordinary non-root path.
- **Alternatives considered**: Folding the row into `interrupts`: it moves a
  standing figure that FR-009 keeps. A table over every label: spec Out of
  scope names the `NMI` line alone, and an arm no test reaches costs a dark
  branch.

## R-5: the span one gap's change answers

- **Decision**: One gap carries the change of the SMI read over the read
  before it: the opening edge read for the window's first gap, and the
  previous gap's read for every later gap. The probe reads at each window
  edge and on the gap path, at no other point of the loop.
- **Rationale**: A gap stands known only after the pass that kept it, so the
  read that ends a span is the read that found the gap, and no read can
  precede the event it measures (spec D-7, FR-013). The keep step stands in
  `keepOrDrop` (`source/harness/jitter_probe.cpp:109-123`), called from the
  two gap loops at `:591` and `:666`. A read per iteration would place a
  system call inside the measured interval, and the effective threshold
  rises with the smallest measured period at `:527-541` above the 200 ns
  default of `include/speedgun-ng/benchmark.hpp:2426`, so every window would
  report its own read as a gap.
- **Alternatives considered**: A periodic background sampler: spec Out of
  scope bars it, and it prices a read no gap owns. One read per iteration
  with a hand-written threshold: FR-012 bars it, and it moves the printed
  `loopPeriodNs`.

## R-6: the kept record and the change ring

- **Decision**: The mark rides a per-cpu ring of 8-byte unsigned changes
  that the gap path appends in the same step as the gap ring, one element
  per kept gap. The kept gap record keeps its 16 bytes and its two fields.
- **Rationale**: `static_assert(sizeof(GapRecord) == 16)` stands at
  `source/harness/detail/jitter_probe.hpp:47` over `stamp` and `gapNs` at
  `:41-45`, and the spec Constraints line keeps that number. `SeriesRecord`
  at `:52-55` with its own assert at `:57` stands the 8-byte ring beside the
  samples, the shape the change ring copies (plan P-3). One capacity and one
  drop counter keep index `i` of one ring equal to index `i` of the other,
  so the sizing at `source/harness/jitter_probe.cpp:477` and the latch copy
  at `:626` keep one pass. `test/source/harness_jitter_probe_alloc_test.cpp`
  runs the loop under an allocation budget, so the ring stands sized before
  the reads.
- **Alternatives considered**: Widening the gap record to 24 bytes: it
  breaks the assert at `:47` and the hot-path cost FR-012 fixes. A side map
  from a gap stamp to a mark: the fold must rebuild it, and one stamp can
  name two gaps.

## R-7: where the three per-cpu figures ride

- **Decision**: `JitterStats` gains `smiDelta` and `nmiDelta` as optional
  counts and `smiGapCount` as a count. One record feeds the run-level
  `jitter.stats[]` items and the `precheck.cpus[].stats` items, and a run of
  any other method writes none of the three keys.
- **Rationale**: `JitterStats` stands at
  `include/speedgun-ng/benchmark.hpp:516` with its 16 standing fields and
  rides inside `PrecheckCpuRecord` at `:685` and `JitterEntryFold` at
  `:763`, so one record already reaches both writers
  (`source/harness/json_report.cpp:332-364`, `:1219-1351`) and the
  comparison (spec D-13, plan P-5). Both objects stand closed with
  `additionalProperties: false` in `docs/schema/report-v1.json`, so a key
  joins `properties`, joins `x-order`, and moves the minor in one commit
  (spec FR-029, D-5). A run that names neither new word writes no new key,
  so `test/source/harness_jitter_bytes_test.cpp` keeps its byte equality.
- **Alternatives considered**: A record beside `JitterStats`: the writer,
  the loader, the console and the comparison would each learn a second
  carrier for three figures. A top-level document key: spec D-13 bars it and
  the root `x-order` would move.

## R-8: the detector directory the kernel makes

- **Decision**: The run never waits for `hwlat_detector/`. It reads and
  writes `window`, `width` and `mode` as plain files under that directory,
  and it starts and stops the sampling through `current_tracer` and
  `tracing_on`.
- **Rationale**: `kernel/trace/trace_hwlat.c:774` creates the directory
  inside `init_tracefs` (`:765`), which `init_hwlat_tracer` calls at `:883`
  behind the `late_initcall` at `:887`, so the directory stands there from
  boot on a host that builds the tracer in. That path places `window` at
  `:778`, `width` at `:785` and `mode` at `:792` inside it, each with
  `TRACE_MODE_WRITE`, and `TRACE_MODE_WRITE` names mode `0640`
  (`kernel/trace/trace.h:34`), so an unprivileged read fails (plan R-5). The
  directory carries no `enable` file, so the engine word stands the only
  start (spec D-9, FR-019), and the lane host shows the absent case (spec
  audit point).
- **Alternatives considered**: A wait loop on the directory: a poll answers
  nothing the write does not answer. A `mkdir` from user space: it writes
  outside the file list FR-019 names and duplicates a layout the kernel
  owns.

## R-9: the start path of the engine

- **Decision**: The run writes the mask, then `mode`, then `width`, then
  `window`, then `tracing_thresh`, then `current_tracer` with the word
  `hwlat`, then `tracing_on` at `1`. Each write answers one result, and a
  failed write ends the setup.
- **Rationale**: The engine word lands on `hwlat_tracer_init`
  (`kernel/trace/trace_hwlat.c:826`, the `.init` slot at `:864`), and the
  sampler starts there only while tracing already stands on (`:842`), so the
  write of `1` reaches the `.start` slot at `:866`. The start reads the mask
  at `:585` and the threshold at `:836`, so both stand written ahead of the
  word (spec D-9, FR-019). `:829-830` answers `-EBUSY` while another tracer
  holds the detector, so a busy host fails the write and states
  `not available` with status 0 (spec FR-023). The standing setup already
  answers a failed write with the empty session
  (`noise_tracer.cpp:313-355`).
- **Alternatives considered**: The engine word before the knobs: the tracer
  reads the standing mask and threshold and watches cpus no rule names. A
  read of `available_tracers` as the probe: it names registered tracers, so
  it cannot separate a busy host from an absent engine, while the write
  does.

## R-10: the two bounds and the three constants

- **Decision**: The run writes three named constants: a window of 10,000 us,
  a width of 5,000 us and a threshold of 1,000 us, and it writes the width
  before the window.
- **Rationale**: `kernel/trace/trace_hwlat.c:53` and `:54` hold the kernel
  defaults of 1,000,000 us for the window and 500,000 us for the width, and
  `:55` holds 10 us for the threshold. `:734` names the standing window as
  the bound of each width write and `:747` names the standing width as the
  bound of the window write, so a width written first cannot fail against
  the standing window (plan R-3). The lane keeps the kernel's one-to-two
  ratio at a span twenty times inside the 200 ms pre-check default, so one
  default run leaves each tracer cpu about twenty windows of its own, and a
  threshold above the kernel's 10 us keeps a short interruption of the
  sampling loop out of the reported gaps (spec A-3, D-10).
- **Alternatives considered**: The kernel defaults: a one second window
  finds no whole window inside the pre-check span, so the record states no
  figure. The kernel's 10 us threshold lets every short scheduler event into
  the gap count, the reading spec A-3 refuses.

## R-11: `per-cpu` mode and the load it buys

- **Decision**: The run writes `hwlat_detector/mode` with the word `per-cpu`
  ahead of the engine word, so each tracer cpu carries its own detector
  thread and states figures of its own.
- **Rationale**: `kernel/trace/trace_hwlat.c:67` names the mode words and
  `:792` creates the file the run writes. The start picks one of two shapes
  at `:810-813`: `start_per_cpu_kthreads` begins one thread per cpu of the
  mask at `:585-591`, and `start_single_kthread` begins one thread at
  `:415-443` that `kthread_fn` moves to the next cpu each round
  (`:360-361`), so a cpu holds no window while that thread sits elsewhere.
  Spec FR-027 reads a zero as a cpu the detector watched and found nothing
  on, a reading the round-robin shape cannot carry. The choice accepts one
  kernel thread per tracer cpu inside the measured window, with interrupts
  disabled through the whole width (`:270`, `:363-365`); FR-031 fixes that
  difference as a line of the guide page (plan P-14).
- **Alternatives considered**: The kernel default `round-robin`: an empty
  buffer answers both a watched cpu and an untouched cpu. One thread on one
  cpu beside per-cpu figures: the figures would name cpus the detector never
  reached.

## R-12: the threshold save and the reverse restore

- **Decision**: The session holds one saved value per file it wrote and the
  restore walks them in reverse, so `tracing_thresh` goes back after the
  engine word. `tracing_on` stands outside the saved set and the restore
  writes `0` there.
- **Rationale**: `kernel/trace/trace_hwlat.c:836` saves `tracing_thresh`
  when the engine word lands and `:857` writes that saved value back at the
  reset at `:850`, so the kernel restores the run's own threshold and only
  the session holds the host's (spec D-8). `:855` records the run's
  threshold as the static default of the next run that names none, a residue
  no user-space write reaches, and the guide page states it (spec FR-036).
  The reverse walk puts the kernel's reset first and the session's write
  last, so the host's value stands at the end of the run (spec FR-021, plan
  P-8). `noiseTracerRestore` (`source/harness/noise_tracer.cpp:375-384`)
  drops each result today, so the restore returns the paths whose write
  failed and the caller prints one line each (spec FR-022, D-12).
- **Alternatives considered**: Restore in write order: the kernel's reset
  then overwrites the host threshold with the run's value, and SC-007 fails
  on that file. A saved value for `tracing_on`: FR-021 fixes the write of
  `0`.

## R-13: the figure source and the line the run parses

- **Decision**: The three detector figures come from
  `sys/kernel/tracing/per_cpu/cpu<id>/trace`, read once at the end of the
  window. `maxGapNs` takes the larger of a line's two sample figures times
  1000, `gapCount` counts the detector lines of that cpu, and `nmiCount`
  sums their `nmi-count` fields and answers 0 where none stands.
- **Rationale**: The detector names no per-cpu statistics file, and the
  kernel creates the buffer file at `kernel/trace/trace.c:7672`, so the text
  under the same root carries the figures (spec FR-018, A-6).
  `kernel/trace/trace_output.c:1531` prints the detection line with
  `inner/outer(us)`, `ts` and `count`, and adds `nmi-count` only above zero
  (`:1538`, `:1546`), so the reader needs a line rule and the sum needs its
  default (plan P-10). `kernel/trace/trace_hwlat.c:296` takes the larger of
  the two samples for its own maximum. The header prints `# tracer:` at
  `kernel/trace/trace.c:3310`, so a rule that needs a digit second after `#`
  skips it (plan R-2).
- **Alternatives considered**: A statistics file: the detector writes none,
  and a file the run creates stands outside the list FR-019 names. Keeping
  the buffer text: spec Out of scope bars a stored trace.

## R-14: the standing figure path names no file

- **Decision**: The lane leaves the standing path as it stands and records
  the finding for spec `036`. The new engine reads the file named in R-13
  and no other.
- **Rationale**: `source/harness/noise_tracer.cpp:367` names
  `sys/kernel/tracing/osnoise/cpu` with the cpu index appended for both
  engines it serves, and the kernel creates no file by that name:
  `kernel/trace/trace_osnoise.c:2772` creates a `per_cpu` directory, `:2778`
  creates one `cpu<id>` directory inside it, and `:2782` creates one
  `timerlat_fd` file inside that directory. Every test plants the standing
  path under a fake root (`test/source/harness_noise_test.cpp:754-757`), so
  no host reading reaches it today, and the plan records the path as an
  inherited observation (plan, post-design re-check).
- **Alternatives considered**: Fixing the standing path here: it moves two
  engines' figures, their console lines and their tests, which spec Out of
  scope bars. Reusing that path for the detector: it names a file the kernel
  never makes.

## R-15: two comparison tables and two refusal lines

- **Decision**: The three probe figures join `kJitterFields`, which moves
  from 32 rows to 35 and keeps the standing `jitter:` grammar. The three
  detector figures stand in a new table of three rows behind a new head
  word, in the shape `hwlat:<cpu|any>:<figure>`.
- **Rationale**: `source/harness/compare.cpp:262` holds `kJitterFields`, and
  one table serves both the name lookup and the value read, so three names
  cost three rows and no new reader (spec FR-028, plan P-11). The count word
  of the refusal line stands at `:429` and in
  `specs/027-jitter-probe/contracts/rule-names.md:31`, so the table and both
  lines move in one commit. The `jitter` head word stands outside the closed
  prefix list at `:55-56` and takes its own branch at `:467-476` into its
  own reader at `:374`, so a second head word costs one branch, one refusal
  line, and its own contract rows. One test plants the standing jitter names
  and requires their refusals word for word (plan R-7).
- **Alternatives considered**: All six names in `kJitterFields`: the
  detector figures ride `PrecheckRecord`, which `compare.cpp` never reads,
  so the table would name a carrier no reader fills. Folding them into
  `JitterEntryFold`: a second carrier for figures the jitter section never
  held (spec D-13).

## R-16: the gates and the readings they hold

- **Decision**: The lane keeps four standing numbers: the DBC pair gate at
  347 interfaces and 0 gaps, the prose gate at 0 findings, the drift gate
  with `README.md` at or under 1,004 lines, and the coverage gate at 100
  percent lines with a branch shortfall at or under 21 rows and a marker
  ceiling of 84.
- **Rationale**: Every cheap gate answered 0 at the base, and the two DBC
  gates measured 347 interfaces with 0 gaps over the Doxygen matrix (spec
  audit point). `tools/docs/example_drift.py:256` holds
  `PAGE_LINE_CAP = 1004` and `README.md` stands at 1,001 lines, so the two
  new words ride the guide page and the main page gains no section (spec
  FR-037, plan R-8). The docs classes at `:491-503` bar a commit hash, a
  kernel reference (`:496`), a spec identifier and a requirement identifier
  from the two user-guide pages named at `:483`.
  `tools/dbc/coverage_gate.sh:102` holds `allowance=21` and `:45` holds the
  strict line rule, against 84 marker lines with 0 under `source/harness/`.
  The prose table bars the contrastive forms at
  `tools/prose/prose_rules.yaml:95` and exempts code spans at `:63`.
- **Alternatives considered**: A coverage marker to reach the branch number:
  SC-002 bars a new marker of any kind. A new `README.md` section: a
  capability section needs two registration entries and a measured cap move,
  against three lines of headroom.

## Open items

- The `msr` read under root. An unprivileged open of `/dev/cpu/0/msr`
  answers `EACCES` on the lane host, so no run here has read a count. The
  root probe run of spec SC-005 settles it and records the node, the value
  at offset `0x34`, and the two window deltas.
- The tracefs writes under root. Every file under `/sys/kernel/tracing`
  answers `EACCES` to this reader, so no run here has written the engine
  word. The root detector run of spec SC-005 settles it, and SC-007 holds
  the before value and the after value of each of the six saved files.
- The three detector constants. Spec A-3 holds 10,000 us, 5,000 us and 1,000
  us as the lane's proposal. The owner rules the three values or keeps the
  proposal, and the plan record names the reading either way.

No other item stands open.
