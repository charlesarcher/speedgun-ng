# Research: Host Noise Pre-Check

Phase 0 output of `/speckit.plan` for
`specs/034-host-noise-precheck/spec.md`. Every NEEDS CLARIFICATION
the plan's Technical Context could carry is resolved here; each
decision names the reading it fixes, the rationale and the
alternatives it rejected.

## R-1: the serial form's public shape

- **Decision**: `JitterProbeOptions` gains
  `bool precheckSerial = false;`, appended after `strict`
  (`include/speedgun-ng/benchmark.hpp:1937`).
- **Rationale**: the suite API binds the probe the way the command
  line does (specs/027 FR-015), so the serial shape needs a public
  carrier. One bool beside the existing fields is the minimum shape;
  the aggregate append is source-compatible for standing callers, and
  the harness `SOVERSION` rises to `13` with the version move to
  `0.21.0` (spec FR-005).
- **Alternatives considered**: a sentinel inside `precheckMs` (a
  `uint32_t` cannot name `serial` without stealing a value the
  bounds grammar forbids); a new `JitterProbeMode` value (the mode
  answers alongside/precheck/both, and the serial shape is
  orthogonal to it, so a mode value would pair two facts in one
  field); a sum type over the two `precheck` forms (a new public
  type for one flag fails the simplicity rule of X.2).

## R-2: the warning rule's fields

- **Decision**: a planned cpu stands noisy when its window recorded
  at least one above-threshold gap; the warning line names the cpu
  and the two figures, the gap rate (`gapsPerSecond`) and the max
  gap (`maxNs`), one line per cpu.
- **Rationale**: the probe records a gap only above the threshold
  (specs/027's `keepOrDrop`), so `maxNs` above the threshold and
  `gapsPerSecond` above zero stand together: the two fields the spec
  names (`gapsPerSecond`, `maxNs`) carry one condition. Naming both
  figures in one line keeps both fields live, costs no second line,
  and leaves the strict refusal free to carry the same two figures
  (spec D-4, the clarify answer of 2026-10-27).
- **Alternatives considered**: a rate threshold derived from the
  threshold (an invented constant the request does not name);
  `maxNs` alone (leaves the spec's gap-rate wording without a
  subject); one line per field (two lines per cpu against the spec's
  "one warning line" shape).

## R-3: the `/proc/schedstat` mapping

- **Decision**: the per-cpu line's run-queue delay figure names the
  context-switch figure, and the timeslices figure names the
  schedule count; each disclosed figure names `/proc/schedstat` and
  its column.
- **Rationale**: the cpu line of `/proc/schedstat` carries three
  figures (run time, run-queue delay, timeslices); the request names
  "the per-cpu context-switch counts (`/proc/schedstat`)" and the
  spec names "the context-switch and schedule counts". The mapping
  above is the one the file's own layout supports without a second
  source.
- **Alternatives considered**: the timeslices figure alone (drops
  one of the two named counts); `/proc/<pid>/status` voluntary and
  nonvoluntary switches (a per-thread source, outside the request's
  file list). The roadmap 10.1 row is re-confirmed at
  implementation; a mismatch goes to the pull request as a question
  (plan R-03).

## R-4: the interrupt delta's aggregation

- **Decision**: the interrupt delta of one cpu is the sum of that
  cpu's column over every row of `/proc/interrupts`, read before and
  after the window.
- **Rationale**: the file carries one count per cpu per IRQ row, so
  the cpu's interrupt activity in the window is the column sum; the
  delta isolates the window's share.
- **Alternatives considered**: per-IRQ deltas (the request names one
  interrupt count per cpu); the first row alone (a hardware-dependent
  reading).

## R-5: the softirq disclosure's granularity

- **Decision**: one delta figure per softirq kind the file states,
  the kind named as the file states it; a kind that cannot stand
  parsed reads `not available` for that kind alone.
- **Rationale**: the clarify answer of 2026-10-27 (spec FR-016): the
  kind is what turns a warning into a cause; an aggregate hides it.

## R-6: the IRQ affinity source

- **Decision**: the affinity reader walks the list form
  `/proc/irq/<n>/smp_affinity_list` and falls back to the mask form
  `/proc/irq/<n>/smp_affinity`; each IRQ whose affinity names a
  measured cpu stands disclosed.
- **Rationale**: both spellings exist across kernels; the list form
  parses without a mask width; the fallback keeps the disclosure
  alive on hosts that state only the mask.
- **Alternatives considered**: `/proc/irq/<n>/smp_affinity` alone
  (a fixed-width mask parse that breaks on large cpuset counts);
  hwloc's own irq tables (a private binding surface the probe does
  not open).

## R-7: the file-root seam

- **Decision**: one detail header,
  `source/harness/detail/proc_root.hpp`: `procRoot()` answers the
  process-global root (`/` by default), `installTestProcRoot(const std::filesystem::path&)`
  installs a root for the test's life and restores the default at
  scope exit; every `/proc` and `/sys` read of the pre-check resolves
  under the root.
- **Rationale**: the seam is the spec's own requirement (D-7): the
  tests install fake files into one root, CI needs no real noise, and
  the N13d and N13e lanes reuse the named seam. `std::filesystem` is
  the standard library, so no new dependency joins (Additional
  Constraints).
- **Alternatives considered**: a virtual-file callback interface (a
  wider seam than the one use needs, X.2); per-source injection
  points (one seam per reader is the drift the spec's "one file-root
  seam" forbids); reusing `installTestOnlineCpus` (it injects a cpu
  set, the files stand a different fact).

## R-8: the window bounding in the tests

- **Decision**: the tests bound every pre-check window through
  `installTestReader` with a scripted `WindowReader` whose stamps
  advance per read; the fake files stand static; the attribution
  deltas come from the fake files' two snapshots.
- **Rationale**: the standing seam (specs/027 R-02) already installs
  a reader; a scripted stamp series closes the window after a chosen
  number of reads, so no test sleeps and no test reads a clock
  (spec FR-011). The timing bounds keep the order-of-magnitude
  margin the record of
  `specs/032-fix-merged-features/results/record.md` names.
- **Alternatives considered**: a real 200 ms window in the tests
  (slow, load-sensitive, against VI); a mocked wall clock (the
  counters-only rule forbids a second time source).

## R-9: the JSON record's placement and keys

- **Decision**: the record stands as a top-level object named
  `precheck`, after `jitter` and before `benchmarks`; the schema
  gains the keys, the top-level `x-order` moves to the list below,
  and the minor moves to `1.6` in the same commit.

  `["schemaVersion", "speedgun", "run", "jitter", "precheck", "benchmarks", "hunt"]`
- **Rationale**: the spec records the placement as an assumption
  (the record follows the jitter record's own pattern); the compare
  major-version rule reads `schemaVersion` alone, so a new top-level
  key stands safe; a run without the pre-check writes no key, so the
  byte-identity rule of FR-024 holds.

## R-10: the measured set and the empty candidate set

- **Decision**: the measured set is the planned cpus (from the
  execution-context plans) plus the candidates (the `idle` preset
  resolution, reused unchanged); the set is computed once before the
  window opens; an empty candidate set stands disclosed and the
  pinning line names the planned cpus alone.
- **Rationale**: the spec fixes the candidates as "the cpus the
  `idle` preset would take" (FR-010), and the resolution exists
  (specs/027 D-4); recomputing it would copy the resolver. The
  clarify answer of 2026-10-27 fixes the empty-set shape.

## R-11: the serial sequencer

- **Decision**: the serial shape runs one `ContextBarrier` round per
  window: the item whose turn it is spins for the span, the others
  wait; the order is the ascending cpu order, the planned cpus
  before the candidates; the measurement pass of every item stands
  before the first window.
- **Rationale**: `ProbeSession` already carries the two
  synchronizers; one extra round per window reuses them, and the
  stats read after `stopAndJoin()` stand independent of the window
  order. No new primitive joins (X.2).
- **Alternatives considered**: a per-item spin gate with an atomic
  turn counter (a second synchronizer shape beside the proven ones);
  running the serial windows from the holder thread (a pinned window
  would land on the wrong cpu).

## R-12: the console block's placement

- **Decision**: `printPrecheckReport` prints after the probe's own
  console block region is reserved and before the benchmark tables:
  one block per measured cpu, then the host-settings disclosure
  once, then the warning lines, then the pinning line, then the
  strict refusal line when the run refuses.
- **Rationale**: the spec fixes "before the benchmark tables" (P-8)
  and the disclosure order "warnings, then pinning" (D-3); the host
  settings stand once per run (P-6), so they print once between the
  per-cpu blocks and the warnings.

## Unresolved items

None. The one externally sourced fact (the roadmap 10.1 row for the
schedstat mapping, R-3) carries its recorded fallback and its
pull-request question path; it blocks no design and no test.
