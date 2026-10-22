# Research: Jitter Probe

Phase 0 output. Every decision carries the requirement it serves, the
reason it was taken, and the alternative that was rejected. The audit
point is `aec8fcef6a3664ec311890930b65ea20541c0fa7`. The clarify
findings of `.omo/waves/027-clarify/q-resolve.md` answer Q-1 (the
provider publishes no runtime invariant-TSC detection: the `tsc` leaf
is a build-time fact and `ClockProvider::open` returns null) and Q-2
(no shared allocation counter exists; each test binary carries its own
counting `operator new` pair).

## R-01: The batch owns the probe contexts; the probe unit is harness-internal

- **Decision**: the probe lives in one harness-internal translation
  unit (`source/harness/jitter_probe.cpp`) beside a private header in
  `source/harness/detail/`, the way the topology read and the preset
  resolver stand behind `detail/internal.hpp`
  (`source/harness/detail/internal.hpp:87`, `:561`). The spinning
  contexts are owned by one holder built in `speedgunMain` beside the
  `ReportData` batch state (`source/harness/cli.cpp:504-509`), started
  before the first `runner.run(*entry)` and stopped, drained and
  joined after the selected loop ends. Each context is constructed as
  `ExecutionContext(ContextPolicy{WaitMode::SPIN,
  PlacementSlot::cpu(n)})`
  (`include/speedgun-ng/execution_context.hpp:91-95`, `:114-125`,
  `:150`), so the pin rides the same default placement function and
  registry every benchmark context uses
  (`source/harness/execution_context.cpp:106`, `:127`). No new public
  header ships beyond the surface FR-015 requires (`JitterProbeOptions`
  and the two entry points beside `BenchmarkHandle`
  (`include/speedgun-ng/benchmark.hpp:1620` and below)).
- **Rationale**: the per-instance context lifecycle the runner owns
  lives and dies with one instance (`source/harness/runner.cpp:1088`,
  the vector and its joiner at `:1090-1103`), but the probe must span
  the whole batch, outside every timed window of every benchmark
  (FR-006, D-7). A batch-scoped holder in `speedgunMain` gives that
  span with no change to the runner's own lifecycle and no second
  context-management path. The pin needs no new mechanism: the slot
  and `PlacementFn` exist exactly for this
  (`include/speedgun-ng/execution_context.hpp:184`, `:201`), and the
  disclosure record of each probe context reads through
  `detail::placementRecordOf` like the threaded run's disclosure does
  (`source/harness/execution_context.cpp:106-127`).
- **Alternatives considered**:
  - Start and stop the probe inside `Runner::run` around each
    instance. Rejected: the probe would stop and restart between
    benchmarks, its own start and stop would sit inside the batch the
    run reports, and a probe gap record could not name a window that
    spans two instances (FR-021).
  - One `std::thread` per probe cpu. Rejected: it bypasses
    `PlacementSlot` and `PlacementFn`, produces no placement record
    the disclosure can print, and builds a second pin path beside the
    one feature 020 and specs/025 already ship.

## R-02: The time seam is the provider catalog; the fake provider scripts it

- **Decision**: the probe reads time only through `sg::counters`: it
  opens a reader over the `machine/tsc` leaf of the `ClockProvider`
  (`include/speedgun-ng/counters_clock.hpp:29-53`, `:62`) through the
  same `System::local()` catalog the runner's time columns resolve
  through (`source/harness/runner.cpp:1259` area, the `machine`
  object and the `monotonic` leaf). Tests script the gap series by
  registering a `FakeProvider` that declares a `machine/tsc` counter
  at `ReadMode::FAST_TSC` and hands the window a scripted cumulative
  point sequence with `setPoints`
  (`include/speedgun-ng/counters_fake.hpp:191`, the `FakeScript`
  fields at `:36-46`); one scripted point per sampling action is one
  probe read, so the gaps, their order and the buffer-full path stand
  under test with no real noise. Where a test needs a series the
  fake's action-count model cannot express (a stamp read that must
  wait on a test barrier), the harness-internal probe unit carries an
  install seam in `source/harness/detail/`, shaped like
  `installTestLayout` (`source/harness/detail/internal.hpp:538`),
  that substitutes the reader the probe opens; the seam installs a
  counters `WindowReader`, never a second time source, so the
  counters-only rule of FR-004 binds both the production path and the
  test path.
- **Rationale**: FR-004 and the Constraints bind the probe to the
  counters surface, and the fake provider is the shipped exactness
  spine for that surface (`include/speedgun-ng/counters_fake.hpp:1-14`,
  the file brief). The clarify run confirmed the fake is an ordinary
  provider over the same window contract, so a scripted `machine/tsc`
  leaf reaches the probe through no new hole. The null-reader
  recovery of Q-1 rides the same seam: `ClockProvider::open` returns
  null for `machine/tsc` on a build without the instruction
  (`source/counters/clock_provider.cpp:311-321`), and the probe takes
  that answer as its availability answer (R-11).
- **Alternatives considered**:
  - A probe-owned function pointer for the stamp read, the
    `setPlacementFunction` pattern
    (`source/harness/execution_context.cpp:127`). Rejected as the
    primary seam: a pointer that returns a raw count is a second time
    source the counters-only rule forbids, and the fake provider
    already answers the scripted-series need. The reader-install seam
    of the decision keeps the test hook inside the counters types.
  - Scripting through `clock_gettime` interception or LD_PRELOAD.
    Rejected: platform-specific, outside the tree's gates, and it
    scripts nothing deterministically.

## R-03: The gap record is 16 bytes; the buffer is pre-sized and first touched on the probe cpu

- **Decision**: one kept gap is one 16-byte record: the stamp of the
  read and the gap in ns, two `std::uint64_t` fields. Each probe cpu
  owns one buffer pre-sized to 65,536 records by default
  (`kDefaultGapCount`), settable through `gaps=<n>` in the range
  1 to 16,777,216 (`kMinGapCount`, `kMaxGapCount`). The buffer is
  allocated and first touched on the probe cpu, inside the probe
  thread's work item, before the probe window opens: the context
  thread applies its pin at thread start before the loop takes a node
  (`source/harness/execution_context.cpp:106`, the pin the default
  placement function performs), so the page faults of the first touch
  land on the pinned cpu and never inside a measured window (FR-009,
  D-6). When the buffer fills, the probe keeps counting the gaps,
  their total and their max, drops the time stamps, and sets
  `droppedGaps`; the fold and the percentiles read only the kept
  records (FR-010).
- **Rationale**: the record size is fixed by D-6, and a fixed-size
  record in a pre-sized vector is the only shape the no-allocation
  loop of FR-005 can write into: one comparison, one store, no
  capacity check beyond the fill test. First touch on the probe cpu
  keeps the soft fault cost off the measured cpus and off the probe's
  own window, which is the whole point of the pre-sizing. The
  count-plus-max bookkeeping for dropped gaps costs no extra storage
  and keeps `droppedGaps` a disclosure the reader sees; the kept
  stamps alone are lost.
- **Alternatives considered**:
  - A ring buffer that wraps and overwrites. Rejected: the window
    split (R-04) needs the kept records in stamp order after the
    stop, and a wrapped ring needs a second pass to reorder; the
    spec's contract is simpler, the kept prefix plus the counters.
  - Per-gap heap allocation behind a growable container. Rejected:
    FR-005 forbids allocation in the probe loop, and the
    no-allocation proof of FR-041 would fail on the growth path by
    construction.
  - Shared atomic counters written from the probe thread during the
    window. Rejected: the runner reads nothing until the probe has
    stopped (R-04), so shared atomics buy nothing and put
    cross-thread traffic on the probe cpu's cache lines.

## R-04: The window split reads the same leaf at the edges; the handoff is join-ordered with the stop flag on its own line

- **Decision**: the runner records the open and close stamps of every
  timed window in the TSC time base the probe reads: one
  `machine/tsc` sample on the benchmark thread at window open and one
  at window close, taken beside the chunk boundaries the recorder
  already tracks (`source/harness/runner.cpp:1774`, `:1916`, the
  `setChunking` calls; the chunk plan at `:1395`). The probe folds a
  kept gap into every window whose half-open `[start, end)` contains
  the gap's stamp, so each benchmark, each repetition, each chunk and
  each pass of a multi-pass run (PR #44, FR-022) receives its own
  share. The handoff between the probe thread and the runner thread
  is order without locks: the probe thread alone writes its buffer
  while it runs; the stop flag is one `std::atomic<bool>` that stands
  in an `alignas` block of its own cache line, read once per loop
  iteration and written once by the runner; the runner reads the
  buffer only after the probe context is joined, the same
  happens-before edge the threaded attempt uses when it reads a
  context's folds after the node's release store
  (`source/harness/runner.cpp:1733`, the barrier rounds; the fold
  read after `launchBodies` settles). The fold and the percentiles
  run after the probe stops (D-7).
- **Rationale**: the split needs one time base, and the gap stamps
  are TSC counts, so window edges taken from the same leaf compare
  directly with no conversion. Edges taken through the monotonic
  leaf would meet the stamps only through the calibration ratio,
  which injects a rate error into every comparison, and the TSC
  entry publishes no rate at all
  (`include/speedgun-ng/counters_clock.hpp:26-33`). The join-ordered
  handoff needs no synchronization object the probe loop would
  touch: the stop flag is the only shared word the loop reads
  (FR-005, D-7), and its own cache line keeps the probe's repeated
  read from sharing a line with fields the runner writes.
- **Alternatives considered**:
  - Convert gap stamps to ns and intersect with the runner's
    monotonic windows. Rejected: a conversion on every comparison,
    and the calibration stands at a process-level estimate with no
    per-comparison guarantee behind it.
  - A mutex or a condition variable on the handoff. Rejected: the
    runner blocks on it after the stop, which puts a wait beside the
    loop and buys nothing over the join edge the context surface
    already provides.
  - A double-buffered swap of the record buffers. Rejected: one
    producer, one consumer, one phase boundary; a swap adds a second
    shared word inside the loop's neighborhood for no capability.

## R-05: JitterStats reuses the nearest-rank percentile and measures its window at two points

- **Decision**: `JitterStats` carries the sixteen fields of
  data-model.md E-03 in the E-03 order. The percentile fields
  (median, p90, p99, p99.9, p99.99) compute through `percentileAt`
  with its nearest-rank rule over the sorted kept gaps
  (`source/harness/percentiles.cpp:104`, the rank at `:115`), the
  same rule the report columns fill through (`:155` per PC-4).
  `gapsPerSecond` divides `gapCount` by the seconds of the covered
  windows' span, the probe's `runtimeNs` at run level and the
  benchmark's window total per benchmark (E-03);
  `totalPercent` divides `totalNs` by the covered duration, the
  probe's `runtimeNs` at run level and the benchmark's window total
  per benchmark (E-03);
  `runtimeNs` is the duration the probe measured between its loop
  start and the read of the stop flag, taken through the same
  `machine/tsc` reader; `loopPeriodNs` is measured before the window
  opens, the minimum loop period the probe observes over the fixed
  measurement pass data-model.md E-05 specifies, 1,024 loop reads on
  the same probe cpu, which also sets the effective threshold when 10
  times that period stands above the configured value (FR-007, D-2).
- **Rationale**: PC-4 binds the probe's percentiles to the shipped
  rule, so one implementation serves both surfaces and the
  nearest-rank behavior a reader knows from the report columns is
  the behavior the jitter fields carry. Measuring `runtimeNs` inside
  the probe (start to stop read) and `loopPeriodNs` before the
  window keeps both disclosure values off the benchmark's timed
  region and makes the threshold rule of D-2 self-contained in the
  probe.
- **Alternatives considered**:
  - A new rank function tuned for large gap counts. Rejected: two
    rank implementations diverge on ties and rounding, and the
    coverage rule would carry both.
  - Counting `totalPercent` against wall time from process start.
    Rejected: the field names the share of the measured window the
    gaps account for; a process-wide denominator mixes probe-free
    time into a per-probe field.

## R-06: One long-only option value, named constants, and a uniqueness test

- **Decision**: `--jitter-probe` enters the option table as a
  long-only entry whose value stands at 256 or above, held as a named
  constant (`kOptJitterProbe`), the shape `getopt_long` returns intact
  for values above the char range
  (`source/harness/cli.cpp:293-312`, the standing table). The grammar
  is the spec's: `<cpu-list|idle>[,threshold=<ns>][,mode=alongside|precheck|both][,precheck=<ms>][,gaps=<n>][,strict]`.
  The value parses once in `speedgunMain`, before the expansion and
  before any run, the point where `--placement` parses today
  (`source/harness/cli.cpp:414-423` area, the process preset parse).
  Every rejection is one line and exit status 2 (FR-038): a bad cpu,
  an unknown preset, a bad threshold, an unknown mode, a bad
  `gaps` value, a bad `precheck` value, no free cpu for `idle`, and
  the alongside overlap.
  One test asserts every value in the option table is unique (FR-011).
- **Rationale**: the option takes no short letter because master's
  letters are taken (`f l r m i w d c M C p P A T j J`,
  `source/harness/cli.cpp:294-312`) and the two merged lanes take
  more (the spec's Parallel work section); a value of 256 or above
  cannot collide with any short letter or with the values of the two
  branches, and the uniqueness test holds that property against
  future edits. Parsing before the first run is what makes the
  status-2 errors cost no measured time, the pattern
  `placementFitStatus` established
  (`source/harness/placement_preset.cpp:173`).
- **Alternatives considered**:
  - A short letter. Rejected: every letter is taken or reserved, and
    the spec states the option takes none (FR-011).
  - Parsing the option value lazily, at the probe's start. Rejected:
    a bad value would print after the first benchmarks ran, which
    breaks the one-line-before-anything-runs contract FR-038
    continues from the standing option handlers
    (`source/harness/cli.cpp:375-384`, the `--json-raw` rejection).

## R-07: The idle resolver is a second pure function; the unpinned mask change rides the process mask

- **Decision**: the `idle` resolution is one pure function beside
  `resolvePreset`
  (`source/harness/placement_preset.cpp:83`), taking the
  `MachineLayout` (`source/harness/detail/internal.hpp:87`) plus the
  set of cpus the selected instances' plans use, and returning one
  cpu per L3 group the plans touch: the lowest cpu no plan uses and
  no SMT sibling of a planned cpu names. The sibling relation reads
  the layout the walk already built: a core's `threads` list holds
  its hardware threads in ascending cpu index
  (`source/harness/detail/internal.hpp:60-72`), so the siblings of a
  planned cpu are the other entries of its core. A group with no free
  cpu is skipped and disclosed; no free cpu at all is the status-2
  stop (FR-016, FR-017, D-4). The overlap check in `alongside` mode
  compares each probe cpu against the planned cpus and their SMT
  siblings (D-3, FR-018). For a benchmark that carries no placement,
  the harness narrows the process affinity mask with
  `sched_setaffinity(0, ...)` to drop the probe cpus for that
  instance's run, discloses the change, restores the mask after the
  instance ends, and stops with status 2 when the intersection
  empties (D-5, FR-019). Tests take synthetic layouts through
  `installTestLayout` (`source/harness/detail/internal.hpp:538`),
  the seam specs/025 Q-5 records.
- **Rationale**: the resolver is pure for the same reason R-03 of
  specs/025 made `resolvePreset` pure: one function holds the whole
  rule, testable without a thread, a pin or the host. The layout
  already carries everything the sibling test needs, so the overlap
  check needs no hwloc and no second read. The process mask is the
  only mask an unpinned benchmark has: its contexts construct with
  `noPin()` (`source/harness/runner.cpp:1328` area, the policy the
  runner builds without a plan), so narrowing the process mask is
  the narrowest change that does what D-5 asks, and the pin path of
  the placement function stays untouched
  (`source/harness/execution_context.cpp:106`).
- **Alternatives considered**:
  - Extending `resolvePreset` with an `idle` preset arm. Rejected:
    the preset table is a five-name contract
    (`source/harness/placement_preset.cpp:75`) and `idle` resolves
    against the plans' used cpus, an input no placement preset
    takes; folding it in would change the resolver's signature for
    every existing caller.
  - Applying the mask per context thread through `PlacementFn`.
    Rejected: a `noPin` slot means no pin by contract
    (`include/speedgun-ng/execution_context.hpp:114-125`), and
    reusing it to mean "pin to a reduced set" overloads the public
    slot with a second meaning the disclosure cannot name.

## R-08: The probe block prints after the tables; the disclosure extends the noise-warning region

- **Decision**: the console probe block prints after the benchmark
  tables, after the last family's fit rows, one column per probe cpu
  listing the `JitterStats` fields in the E-03 order (FR-023). The
  per-benchmark line
  `jitter: worst cpu <n> gaps=<k> max=<ns> total=<percent>`
  prints under that benchmark's rows inside `printResult`
  (`source/harness/report.cpp:331`), beside the context disclosure
  region that already prints below a threaded result's rows
  (`source/harness/report.cpp:516` area). The run-level disclosure
  (probe running and on which cpus, the loop period and the
  threshold, `droppedGaps`, the skipped groups, the affinity-mask
  change, the kept-gaps-only statement) prints as new lines in the
  same end-of-report region `printNoiseWarning` owns
  (`source/harness/report.cpp:193`, called at `:516`;
  `source/harness/detail/json_report.hpp:137`, `:154`), and replaces
  nothing: the noise-warning condition and its three fields keep
  their bytes (PC-6, FR-025, D-8).
- **Rationale**: FR-023 fixes the block's position, and the
  noise-warning region is where a reader already looks for
  run-hygiene disclosures, so the probe's disclosure lines stand
  there and no existing line moves. The per-benchmark line carries
  the attribution a reader acts on (User Story 3) at the row it
  belongs to; a block the reader joins against the tables by name
  would hide the one line an engineer acts on.
- **Alternatives considered**:
  - Folding the probe fields into the `contexts=` disclosure line.
    Rejected: that line's shape is fixed by three contracts, and a
    probe run would then change the bytes of every threaded run
    even with no probe, which FR-013 forbids.
  - Printing the probe block before the tables. Rejected: FR-023
    says after, and the numbers read as a footnote to the run, not
    as its headline.

## R-09: The JSON shapes extend the v1 schema in place; the raw gap list rides the existing option

- **Decision**: the document gains a run-level `jitter` object at the
  root, after `speedgun` and before `benchmarks`
  (`docs/schema/report-v1.json:7`, the root `x-order`), holding the
  mode, `thresholdNs`, `loopPeriodNs`, the probe cpus, the stats of
  each probe cpu, `droppedGaps` and the disclosure text (FR-026).
  Each benchmark entry gains a nullable `jitter` object after
  `noiseWarning`
  (`docs/schema/report-v1.json:102`, the entry `x-order`), holding
  the per-benchmark stats over that benchmark's windows (FR-027).
  The writer extends the `JsonObjectWriter` scopes of
  `writeReport` (`source/harness/json_report.cpp:518`), and the
  reader extends `readEntry`/`readRow` with the same optional-key
  pattern `contextPercentiles` and `raw` already use
  (`source/harness/json_load.cpp:1101`, `loadReport`). The schema
  checker compares key set, key order and value kinds
  (`source/harness/json_report.hpp:186` area, `checkReport`), so the
  schema rows and the writer move in the same commit. `--json-raw`
  gains the value `gaps` beside `chunks` and `none`
  (`source/harness/cli.cpp:309`, `:375-384`; the
  `RawContent` carrier at
  `source/harness/detail/internal.hpp:456`), and only that value
  writes the gap list (FR-028).
- **Rationale**: the document is versioned by major version only, and
  the reader refuses only a different major
  (`source/harness/json_load.cpp:1101` area, the version check), so a
  minor-shaped addition rides version 1 with the closed-object
  schema updated, which is exactly what the schema checker enforces.
  Reusing `--json-raw` keeps the raw-content surface one option with
  one closed value list, the shape the N3 option established.
- **Alternatives considered**:
  - A second schema version for the jitter fields. Rejected: the
    reader's version contract accepts every 1.x, so a version bump
    would buy nothing and split the test matrix for no reader gain.
  - Writing the gap list unconditionally. Rejected: FR-028 forbids
    it, and the list can reach 16,777,216 records per cpu, which is
    not a document a reader should receive by surprise.

## R-10: One mode enum, two phases, one probe

- **Decision**: the mode is one enum (`ALONGSIDE`, `PRECHECK`,
  `BOTH`) inside `JitterProbeOptions`; `alongside` is the default
  when `--jitter-probe` names no `mode=` value (D-1, FR-012). The
  run executes at most two phases over one probe cpu set and one
  buffer set per cpu: the pre-check phase (all chosen cpus spin at
  the same time for 200 ms, `precheck=<ms>` setting the duration,
  D-13) runs before the first benchmark when the mode is `PRECHECK`
  or `BOTH`; the alongside phase (the probe spans the batch, R-01)
  runs when the mode is `ALONGSIDE` or `BOTH`. The overlap check
  binds the alongside phase only, so a `precheck` or `both` run may
  use the benchmark cpus for the pre-check (D-3, FR-018).
- **Rationale**: the phases share the threshold, the buffers, the
  disclosure and the availability answer, so two phase executions of
  one probe need no second instance of any of them; `both` composes
  the two phases and adds no third behavior
  (FR-014). The overlap exemption falls out of the phase split: the
  pre-check stops before the first benchmark starts, so sharing a
  cpu with a benchmark context corrupts nothing.
- **Alternatives considered**:
  - Two probe instances for `both`, one per phase. Rejected: double
    the buffers and double the spinning contexts on the same cpus,
    which fight each other in the pre-check window the spec means
    to measure cleanly.
  - Two independent bools (`precheck`, `alongside`) on the options
    struct and the CLI. Rejected: the three-state grammar with an
    unknown-mode rejection (FR-040) needs one closed value set, and
    the API rule FR-003 keeps bools out of the public entry points.

## R-11: The probe answers availability from the catalog and the null reader

- **Decision**: at probe start, before any benchmark runs, the probe
  checks the provider catalog for the `machine/tsc` leaf and opens a
  reader over it. The leaf is a build-time fact: the catalog push
  stands inside `#ifdef SG_COUNTERS_X86`
  (`source/counters/clock_provider.cpp:279-293`, the gate at
  `:45-56`), and `ClockProvider::open` returns null for a
  `machine/tsc` request on such a build
  (`source/counters/clock_provider.cpp:311-321`). A missing leaf or a
  null reader makes the probe disclose "not available" and the run
  continue with status 0; only `,strict` on the option value fails
  the process with status 2 (FR-037, D-10, Clarifications Q-1).
- **Rationale**: the clarify run established that master publishes no
  runtime invariant-TSC detection: no `Availability` enumerator
  describes the absence, and every clock seed sets
  `Availability::COUNTABLE` (`.omo/waves/027-clarify/q-resolve.md`,
  Q-1). The catalog and the null reader are therefore the only
  signals the surface carries, and both are recoverable; that is
  why the non-strict path continues the run and only strict fails
  it.
- **Alternatives considered**:
  - A new `Availability` enumerator for the absent TSC. Rejected: it
    moves a public enum, the closed word table of the document
    (`source/harness/detail/json_report.hpp:75-84` area,
    `kAvailabilityWords`) and the schema enum
    (`docs/schema/report-v1.json:117` area), a layout and contract
    move the feature does not need when the catalog already carries
    the fact.
  - A CPUID check at probe start. Rejected: master publishes none,
    the build-time gate is the documented catalog fact (Q-1), and a
    second detection path would disagree with the provider's own
    answer on exactly the builds where it matters.

## R-12: The rule names ride the merged N5 parser

- **Decision**: the value names `jitter:<cpu>:<field>` and
  `jitter:any:<field>` join `parseCompareRule(std::string_view)`
  (`origin/026-regression-gate`,
  `include/speedgun-ng/benchmark.hpp:2500`;
  `source/harness/compare.cpp:722`), with the field names of FR-002
  in camelCase and the `jitter:run:` prefix naming the whole-run
  values where a bare `jitter:` names the per-benchmark values
  (D-11, FR-030 to FR-032). The branch waits for N5 and rebases; it
  copies no parser code (FR-033). A `jitter:` rule evaluated on a
  report with no probe is the parser's usage error, one line and
  status 2.
- **Rationale**: the merged N5 parser stands as public API with the
  option `--compare-rule` already in the table, so the probe's names
  are new value-name producers over an existing surface, and the
  comparison gate of User Story 4 needs no option of its own. The
  wait-and-rebase rule keeps one parser: a probe-local parser would
  diverge from the N5 grammar on exactly the inputs the gate reads.
- **Alternatives considered**:
  - A private mini-parser in the probe unit until N5 lands.
    Rejected: two parsers over one value-name grammar is the
    divergence FR-033 exists to prevent, and the coverage rule
    would carry both.
  - A separate `--jitter-rule` option. Rejected: D-11 puts the names
    on the N5 value-name surface, and a second option would split
    the comparison run's rule list across two parsers and two
    tables.

## R-13: Four test executables, one counting-new proof, barriers and no clock

- **Decision**: four new files follow the
  `test/source/harness_*_test.cpp` convention and join
  `test/CMakeLists.txt` with the standing
  `add_executable`/`add_test` pair
  (`test/CMakeLists.txt:9-13`, `:624-628`):
  `harness_jitter_probe_test.cpp` (the loop: every branch of gap
  detection, the buffer-full path, the threshold rule including the
  10x loop-period floor, and the no-allocation proof),
  `harness_jitter_window_test.cpp` (the window split, the multi-pass
  split, per-benchmark attribution, the kept-gaps-only rule),
  `harness_jitter_preset_test.cpp` (the `idle` resolver, the overlap
  check, the unpinned-mask change, over synthetic layouts installed
  through `installTestLayout`,
  `source/harness/detail/internal.hpp:538`), and
  `harness_jitter_cli_test.cpp` (the option grammar, every status-2
  case, the option-value uniqueness test, the mode machine). The
  no-allocation proof writes its own file-local replaceable
  `operator new`/`operator delete` counting hooks and diffs the
  count around the region under test, in the pattern of
  `test/source/harness_capture_test.cpp:33-44` and the four siblings
  the clarify run lists (FR-041, Clarifications Q-2). Start and stop
  synchronize on barriers; no test sleeps, reads a clock, holds a
  fixed cpu number, or skips: cpus come from `sched_getaffinity`,
  each below the online count (FR-039, FR-040).
- **Rationale**: one executable per area keeps the failure surfaces
  apart and lets the pure resolver test run without the harness
  entry point, the reasoning R-11 of specs/025 recorded for the
  placement executables. The counting pattern is per-file by
  necessity: the clarify run confirmed the harness library carries
  no shared allocation counter, so each test binary owns its pair
  and diffs its own count (Q-2). The time series is scripted, so
  every branch runs on every host and the suite passes on a 4-cpu CI
  runner and on a 2-cpu host alike.
- **Alternatives considered**:
  - One combined jitter test executable. Rejected: the coverage and
    sanitizer runs would lose the per-area granularity, the failure
    surface would merge, and the CLI test needs the option table
    while the resolver test needs none of it.
  - A shared allocation-counting fixture in the harness library.
    Rejected: no such counter exists on master (Q-2), and adding one
    to the library is a public-surface move this feature's test seam
    rule does not ask for.

## R-14: The README section fits the default cap; the example carries an `.expected` of host-independent fields

- **Decision**: the README section "Jitter probe" runs at most 25
  lines with its quick-reference rows and adds no entry to
  `SECTION_LINE_CAPS`, because the default cap 25 applies
  (`tools/docs/example_drift.py:133`, the map at `:138`). The section
  joins `SECTIONS` and `SECTION_OWNERS`
  (`tools/docs/example_drift.py:30` area). The example
  `example/docs/jitter_probe.cpp` builds through `add_docs_example`
  as the `docs_jitter_probe` target with its `run_docs_jitter_probe`
  runner, beside the twenty-three docs examples registered today
  (`example/CMakeLists.txt:37-71`), and `test/example_run.sh` runs
  it; its `.expected` file checks only the fields with no host
  dependence: the block header, the field names, and the cpu count
  rule (FR-035, US5). `docs/pages/harness.md` carries the full
  option table and the field reference. At the rebase,
  `PAGE_LINE_CAP` moves from 713 to the measured length of the page
  with the new section and its rows, through the recorded-decision
  comment the specs/023 and specs/025 moves precede
  (`tools/docs/example_drift.py:122-132`).
- **Rationale**: the section fits 25 lines when the detail moves to
  `docs/pages/harness.md`, which is the same split the metric
  section took in the PR #41 review recorded in the same file, so
  no per-section exception is needed. The `.expected` scope follows
  the pattern of the other docs examples whose output carries host
  dependence: values the host decides (gap counts, ns fields) stand
  outside the check, and only the shapes every host prints stand in
  it, so the example runs green on the 4-cpu CI runner and on
  thrawn.
- **Alternatives considered**:
  - A `SECTION_LINE_CAPS` entry raising this section's cap.
    Rejected: FR-034 forbids the entry, and a section that needs a
    raise is a section that belongs on the detail page.
  - An `.expected` file checking the probe's numeric fields.
    Rejected: every number the probe prints depends on the host's
    noise, the exact dependence the acceptance scenario names, and
    the check would fail on any second machine.

## R-15: Names follow V.1; the options struct replaces the bool

- **Decision**: new types take PascalCase (`JitterProbe`,
  `JitterProbeOptions`, `JitterStats`, `GapRecord`), new functions
  take camelBack (`resolveIdleCpus`, `gapStatsOf`), new constants
  take the `k` prefix
  (`kDefaultGapCount`, `kMaxGapCount`, `kMinGapCount`,
  `kDefaultThresholdNs`, `kDefaultPrecheckMs`, `kOptJitterProbe`),
  and new files take lower_case (`jitter_probe.cpp`,
  `jitter_probe.hpp`), matching the shipped surface: the PascalCase
  types of `include/speedgun-ng/execution_context.hpp`
  (`:91-125`), the `k`-prefixed tables of
  `source/harness/placement_preset.cpp:47-54` and
  `source/harness/percentiles.cpp:24-26`, the camelBack
  `percentileAt` (`source/harness/percentiles.cpp:104`) and the
  lower_case file names of `source/harness/`. The public API in
  `include/speedgun-ng/` takes `JitterProbeOptions` and no bool: the
  suite-level call and `BenchmarkHandle::jitterProbe(...)`
  (`include/speedgun-ng/benchmark.hpp:1620` and below, beside the
  fluent setters through `:1726` and the `placement` and
  `threadRange` forms at `:2102`, `:2120`) each take one
  `const JitterProbeOptions&`. The struct's members are named
  fields; the strict flag is one of them, because the no-bool rule
  binds the entry-point signature, and the record the entry point
  receives carries its fields freely.
- **Rationale**: FR-003 states the API rule, and the struct is what
  makes it hold: adding a field later moves no signature, while a
  bool parameter would freeze the call shape at its first spelling.
  The naming rules are Principle V.1's, and every surface the
  feature touches already follows them, so the new names read as
  the tree's own.
- **Alternatives considered**:
  - `jitterProbe(bool strict)` and per-flag bool parameters on the
    handle. Rejected: FR-003 forbids the bool, and each new flag
    would move the fluent form's signature again.
  - A builder type for the options. Rejected under Principle X: one
    aggregate with default member initializers serves both entry
    points, and no second construction path exists to justify the
    indirection.

## Open items resolved by this phase

| Spec item | Resolution |
| --- | --- |
| Where the probe lifecycle and its contexts live | R-01 |
| Which seam scripts the time series for tests | R-02 |
| The gap record layout, the buffer bounds and the buffer-full path | R-03 |
| The window edge time base and the probe-to-runner handoff | R-04 |
| The percentiles and the two runtime measurement points | R-05 |
| The option value constants, the grammar and the uniqueness test | R-06 |
| The idle resolver shape, the overlap check and the unpinned mask change | R-07 |
| The console block position and the disclosure extension | R-08 |
| The JSON keys, the schema move and the raw gap list | R-09 |
| The mode machine and the pre-check scope | R-10 |
| The availability answer on a build without the TSC leaf | R-11, Q-1 |
| The parser integration point for the rule names | R-12 |
| The test executables and the no-allocation proof | R-13, Q-2 |
| The page cap move and the example's `.expected` scope | R-14 |
| The naming rules and the options-struct API | R-15 |
