# sg_counters — discussion journal

**Purpose:** working notes that become the initial prompt for
`/speckit.specify` of the counters-and-timers library (future spec
`007-counters-and-timers`).

**Status:** CLOSED 2026-09-25. Hand for `/speckit.specify`: the
**Resolved scope statement** section at the end (self-contained).
Older axis headers may read OPEN; where they conflict with the
decision entries or the scope statement, the scope statement wins
(A2/A3/A4 mechanics per leaf kind; A9→A13/A16/A17; A12→A22;
A15→A17; A5/A2-TSC→deferred list).

---

## Repo state at discussion start (2026-09-25)

- Specs 001–006 done: DBC facility, prose lint, vendored hwloc,
  simdjson, hdrhistogram_c, yaml-cpp (plus zlib).
- Public surface today: `include/speedgun-ng/dbc.hpp` only. No
  measurement code yet. The vendored deps are load-bearing ahead of
  any user-facing feature — they were staged for this.
- Implied architecture from the vendoring: hwloc for topology/pinning,
  HdrHistogram_c for latency distributions, simdjson + yaml-cpp for
  report output, zlib for compressed artifacts.

## What counters-and-timers is

The first substantive layer of speedgun-ng: the measurement primitives
everything above it (harness, reporting, calibration) is built from.

## Design axes (closed: A1–A4 mechanics per leaf kind, rest below)

### A1 — Layer position
Primitive library only (timers, counters, no user-facing harness), or
does this spec also define the registration/run API a user touches?

- **Lean:** primitives only. Harness is spec 008+. Keeps 007 testable
  in isolation and keeps the spec small enough to plan.

### A2 — Clock sources
Which time bases must the timer abstraction cover at v1?

- Wall / monotonic (`steady_clock`-grade).
- Thread CPU time (`clock_gettime(CLOCK_THREAD_CPUTIME_ID)`).
- Process CPU time.
- Hardware TSC / `__rdtsc` — raw cycle counter, needs calibration or
  known-frequency assumption.

- **Lean:** monotonic + thread CPU first; process CPU trivial add;
  TSC gated behind its own requirement because calibration drags in
  a whole subsystem.

### A3 — Timer shape
- Scoped/RAII (`{ sg::timer t(rec); }`)?
- Manual start/stop, restartable, pausable?
- Single-shot vs accumulated laps?
- Overhead calibration: measure the clock's own read cost and expose
  a subtractable floor?

- **Lean:** RAII scoped + manual start/stop both; laps deferred;
  overhead calibration is a requirement (benchmark frameworks that
  skip it lie at the low end).

### A4 — Counters
- Monotonic increment counter (operations completed)?
- Rate/throughput derived from counter + timer window?
- Concurrency: per-thread shards with fold, or plain `atomic<uint64>`?
- Overflow: `uint64` wrap assumed impossible-in-practice, or detected?

- **Lean:** counter = `uint64` with relaxed atomic add; sharding is a
  perf concern for a later spec; overflow undetected, documented
  (2^64 at GHz rates still takes ~195 years).

### A8 — Unified counter model (COMPOSITE) — DECIDED 2026-09-25
Owner directive: timers and counters are one concept. A counter counts
events of a class over a window; a timer is a counter whose events are
clock ticks. The generalization is the design.

Consequences:
- Core abstraction: a **count source** leaf — anything that yields a
  delta over a measurement window (clock ticks, PMU events,
  user-incremented events).
- **Composite counters**: algebra over leaves — sum, difference,
  scalar scaling, and especially ratio — build derived metrics.
  IPC = instructions / cycles: instruction-count leaf ÷ cycles leaf.
- The algebra operates on **same-window deltas**, not point values.
  A composite is meaningless unless every member counts the same
  measurement scope. That makes the measurement scope object (start
  all leaves, stop all leaves, fold) the real central type.
- A3's scoped-timer collapses into this: scoped timer = a scope with
  one clock leaf. `sg::timer` may survive as a convenience alias.
- Prior art sanity: Linux `perf` metric expressions
  (`instructions/cycles`) work this way; PAPI event sets likewise.

### A9 — PMU leaves — RESOLVED by A13/A16/A17
"Counts instructions" is not available from libc. Instruction counts
need hardware performance counters: `perf_event_open` on Linux
(per-thread, gated by `perf_event_paranoid`), effectively unavailable
on macOS (no user PMU API on Apple Silicon) and awkward on Windows.
Questions: is a PMU leaf in 007 Linux-only with a clean
`unsupported` result elsewhere? And a design constraint either way:
composite metrics with more leaf events than physical counters force
multiplexing (scaled estimates, error) — expose group limits as a
contract?

### A21 — Scope owns the snapshot — DECIDED 2026-09-25 (Option A)
The scope is the record of one window in time and the sole snapshot
owner. Composites are formulas + views that fold a scope's stored
deltas; they never read hardware themselves.

- Construction: scope unions the leaf needs of all registered
  composites, lays out one PMU group, compiles the read plan.
- `finish()`: one group read + clock leaf reads + relaxed loads →
  snapshot of all deltas, one instant of truth.
- Any composite's `.metric()` folds from that snapshot. Composites
  sharing leaves share identical deltas — cross-metric agreement is
  structural, not luck.
- Single-composite sugar `ipc.start(); ...; ipc.finish();` is exactly
  a scope containing only `ipc`; one semantics, no special case.
- Anti-pattern prevented (why Option B was rejected): per-composite
  begin/end = double hardware reads and divergent windows for metrics
  the user believes were "the same run".

### A10 — Unit system for the algebra — DECIDED 2026-09-25 (option a)
Two compile-time dimensions: `time^t × events^c`, carried in the
counter/composite type, checked at construction, erased at read time
(zero A19 cost).

- `+`/`-` require identical tags; `/` subtracts exponents; `×` adds;
  scalar scale free. `bytes + monotonic` is a compile error.
- Limitation accepted and documented: `events^1 + events^1` (wrong
  which-count) still compiles. Catches the time-vs-count class — the
  one people actually make.
- Option b (per-event-kind units) rejected: template hostility,
  cast-escape-hatch erosion. Option c (plain double) rejected:
  nonsense ships.
- Resolution mapping: catalog JSON `Unit` → tag via a closed switch
  (`seconds`→time^1; `none`/`ops`/count-units→events^1; unknown →
  resolution error, never a guessed type). The switch is the review
  surface for accepted units.

### A20 — Raw access + lazy metric evaluation — DECIDED 2026-09-25
Owner directive: a composite must expose **every constituent raw
counter**, and the algebraic metric is **evaluated only on demand**,
via a `.metric()` routine. Construction of the algebra performs no
reads; nothing computes the fold that the caller did not ask for.

Model:
- Algebra builds the expression (typed, A10-checked) — pure setup.
- Scope boundaries refresh **raw leaf deltas** (uint64 snapshots) in
  the compiled plan's slot array. This is the only hardware-touching
  step (A19 budget).
- `.metric()` folds the stored deltas — O(ops) doubles, zero syscalls,
  callable any number of times after the fact, callable for any
  subset. Re-asking never re-reads hardware; a new scope boundary is
  what refreshes inputs.
- Raw access: composite exposes each leaf by catalog name/handle →
  `{ name, description, unit, raw_delta, multiplex_ratio }`. Full
  provenance, always available — the metric is a *derived view* over
  inspectable parts, never a black box. Report layer (008) consumes
  raw + metric together: "IPC 1.31 ← instructions 12.3e9 / cycles
  9.4e9, running ratio 0.98".

`.metric()` return is a small struct, not a bare double:
`{ double value; double running_ratio; bool scaled; }` — the
multiplexing disclosure (A16 §7) must be structurally impossible to
ignore at the point a number enters a report. `.value()` sugar for
callers who insist.

Resolves **A11 (value model)**: raw deltas `uint64_t` (clock ticks are
counts too); fold promotes at first division/scaling to `double`;
fold plan orders early-division to keep magnitudes bounded. Document,
not hide: integer counts beyond 2^53 lose exactness in the double
fold — at 1e9 counts/sec that is ~104 days of continuous counting in
one scope. Scopes that long are aggregates, not measurements.

### A11 — Value model of the algebra — RESOLVED by A20
Leaf deltas `uint64`; promotion at fold to `double`; raw kept for
provenance. See A20.

### A12 — Push vs pull leaves — RESOLVED by A22/A25
(both are cumulative-stream leaves behind the provider contract)
- **Pull/snapshot**: source read at scope open/close (clocks, PMU).
  User code untouched.
- **Push**: user code increments in the hot path
  (`c.add(n)` — operations done, bytes sent).
Both are count sources; composites mix them freely
(e.g. bytes/sec = push byte-counter ÷ clock leaf).

### A13 — `system` abstraction: catalog-driven counters — DECIDED 2026-09-25
Owner directive, inspired by andikleen/pmu-tools. Platform details are
abstracted behind a **system** object. The system is queried and
returns a list of **named counters with descriptions**; the user
composes metrics from that catalog by name.

pmu-tools mechanics to borrow (verified against repo 2026-09-25):
- **jevents** — the C library, closest analogue for us: resolves
  named events (`INST_RETIRED.ANY`) to `perf_event_attr`; higher-level
  functions for self-profiling from C programs; `jestat` as perf-stat
  clone. Named-event resolution over data tables, not hardcoded
  constants.
- Event dictionaries derive from `intel/perfmon` JSON (pmu-tools
  downloads on first run, caches for offline). Per-arch tables: event
  name, unit masks, descriptions, scaling. Unit masks/config bits
  never surface to the user.
- `ucevent` computes higher-level metrics derived from multiple
  events — the composite pattern (A8) proven at the uncore level.
- `ocperf` supplies the *full* event list for a CPU, not just perf
  builtins — enumeration beats builtin assumptions.

Shape for speedgun-ng:
- `system::enumerate()` → catalog entries: `{name, description,
  unit-dimension, source-platform}`. Catalog includes **all** leaf
  kinds on equal footing: clock ticks (time unit), PMU events where
  available (count unit), and synthetic/push counters.
- Name resolution: string → leaf. Typos fail with a catalog-driven
  diagnostic (did-you-mean over descriptions), not a syscall errno.
- Composition (A8 algebra) then operates on resolved leaves.
- Platform abstraction: Linux = real PMU catalog (perf sysfs
  `/sys/bus/event_source/devices/*/`, possibly perf-core JSON
  vendor tables) merged with clock leaves. macOS/Windows = reduced
  catalog (clock ticks, push counters) — same interface, shorter
  list, no lying. The catalog *is* the capability report.

Consequences:
- A9 reframed: PMU leaves are not hand-coded event tables in our
  source; they are discovered data. Our Linux backend maps
  `perf_event_open` config from the catalog.
- hwloc (already vendored) is a natural client of the same system
  object: topology + frequency metadata as system queries.
- Metric *definitions* become data too (name → expression). Whether
  users author expressions as strings or as C++ algebra: open (A14).

### A14 — Composition front-end — DECIDED 2026-09-25
Owner call: **(a) C++ algebra.** `sys["instructions"] / sys["cycles"]`.
String formulas deferred to 008; grammar pinned now as perf
MetricExpr-subset (no second speedgun dialect when they arrive).
MetricExpr catalog rows stay dead data until 008 ingests them.

**Hard performance constraint (owner):** counter/composite
*construction* is not on the critical path; *reading* a counter or
composite absolutely is. Design consequence in A19.

### A19 — Construction/read path split — DECIDED 2026-09-25
Two phases, contractually separate:

**Setup (slow allowed):** catalog name resolution (string map,
did-you-mean diagnostics), expression-tree building, dimension
checking (A10), PMU group layout, scratch allocation, plan
compilation. A composite compiles once into a flat read plan —
leaf slots + fold sequence — no tree, no vtable, no closures, no
lookups at read time.

**Read path budget (the contract):**
- Clock leaves: 2 × vDSO `clock_gettime` per scope ≈ 40–50 ns.
- PMU leaves: **one** `read()` per group per scope boundary, not per
  leaf. Group leader fd + `PERF_FORMAT_GROUP` returns every member's
  value *plus* `time_enabled`/`time_running` — multiplex ratio for
  free, zero extra syscalls. ~0.5–2 µs, the irreducible cost.
- Push leaves: relaxed atomic load, ~ns.
- Fold: doubles over a fixed array; preallocated result slot; no
  malloc, no lock, `noexcept`, branchless where the fold allows.

Honest consequence, documented not hidden: a composite containing PMU
leaves has per-scope cost dominated by the group read — sub-µs
scopes cannot measure it. Clock-only scopes stay in the tens of ns.
A3's overhead calibration measures exactly this floor per composite
plan, so every number a user reports carries its own measurement
overhead.

Grouping constraint (kernel rule, becomes a construction-time
contract): events in one PMU group must share thread/cpu target and
clock_id. Construction validates it; mismatch = construction error,
never a surprise at read.

> Correction (review pass, 2026-09-25, post-A25): "one group read"
> is per-PMU, not per-sample — kernel groups do not span PMU
> boundaries; a plan touching core + uncore PMUs pays one `read()`
> per PMU leader per `sample()`. Budget scales with distinct PMUs
> in the plan. Scope statement carries the corrected wording.

Hot-loop pattern (documented idiom): leaves accumulate inside the
loop; composites are read at scope boundaries, never per iteration.
PMU counting is scope-granular physics, not an implementation flaw.

### A15 — Catalog provenance — RESOLVED by A17 (bundled ∪ sysfs ∪ distro merge)
Where do descriptions/event tables come from?
- (a) OS-provided only: perf sysfs `/sys/bus/event_source/devices/*`
  (format dirs, some alias events with descriptions) and
  `/usr/share/perf-core/vendor_events/*.json` when the distro
  provides them. Zero bundled data; coverage varies with distro.
- (b) Bundle per-arch event tables, jevents-style, derived from
  `intel/perfmon` JSON (BSD+ licence — check redistribution terms).
  Full descriptions everywhere; tables age per microarch.
- Note: (b) *is* the house style. Every vendored input already has a
  version gate (`hwloc_gate.cpp` et al). An `event_table_gate` with a
  pinned perfmon snapshot commit fits the constitution exactly.
- Hybrid likely best: merged catalog = bundled tables ∪ sysfs
  discovered (covers AMD/uncore/new CPUs the table lacks). sysfs wins
  on conflicts.

### A17 — Obtaining the rich layer — PROPOSED 2026-09-25
Owner call: our catalog *is* the rich layer; we do not inherit the
distro's thinness. How to obtain it:

**Source: the Linux kernel's `tools/perf/pmu-events` tree.** It is
canonical, covers Intel *and* AMD, and includes `mapfile.csv`
(CPUID-signature → table-directory regex map — perf's own resolution
mechanism, free with the tree). Licensing: `tools/perf` is dual
GPLv2-or-later/MIT; the JSON data is therefore BSD-3-compatible —
confirm in a legal pass before vendoring. Rejected alternatives:
- intel/perfmon alone: cleanest licence (BSD+), but Intel-only —
  thin catalog on the author's AMD box. Dead on arrival.
- pmu-tools itself: GPL-2.0, Python + C mix; we want the data, not
  the tree. jevents is prior art for the mechanism, not a dependency.
- Runtime download (pmu-tools behaviour): never. Reproducibility and
  the vendoring constitution forbid it.

**Acquisition mechanism** (fits the existing re-pinning house style):
1. Snapshot the `tools/perf/pmu-events` path at a pinned kernel
   commit — cgit archive of the path is a few MB of JSON, not the
   whole 140 MB tarball. Vendor under `external/pmu-events`.
2. Version gate, same tripwire pattern as `zlib_gate.cpp`: assert the
   recorded kernel commit/tag matches the vendored tree. Re-pinning
   documented in README beside the hwloc/simdjson sections.

**Build/runtime pipeline:**
```
pin kernel commit            (gate asserts pin == vendored tree)
CPUID family/model           --mapfile.csv-->  arch table dir
simdjson (vendored) parse    --> name, BriefDescription, EventCode,
                                 UMask, Unit, Scale, metric rows
sysfs format/<field> bits    --> attr.config encoding
perf_event_open probe        --> availability state per entry
distro /usr/share/perf-core/vendor-events, when newer --> merge
```
Key decoupling (jevents does exactly this): JSON carries *semantic*
event code + mask; sysfs `format/` carries *this kernel's* bit
layout; config = compose. A JSON table older than the kernel ABI
still encodes correctly. If the kernel lacks a needed format field,
that event is `not-encodable-here` — honest availability.

Parse cost: tables are a few MB total; parse lazily, only the
CPUID-matched arch dir, once at system-open. simdjson makes this
noise.

Metric rows (`MetricName`/`MetricExpr`/`MetricGroup`) come along for
free — the seed catalog for composite named metrics (A14) ships in
the same data.

### A18 — Upgrade utility for the vendored tables — DECIDED 2026-09-25
Owner directive: the JSON tables need a first-class upgrade tool, not
a README recipe. House precedent: `tools/prose/` (python3, `--check`
mode, ctest fixtures).

`tools/pmu_events/update_pmu_events.py`, two modes:

- `--to <kernel-ref>` (upgrade):
  1. Fetch `tools/perf/pmu-events` at the ref from kernel.org cgit
     path archive (`archive.tar.gz?path=...` — path only, no kernel
     clone). GitHub mirror as documented fallback.
  2. Strip to scope: x86 Intel + AMD table dirs and `mapfile.csv`;
     other arches excluded explicitly (recorded exclusion list, not
     silent absence).
  3. Validate before replacing: every file parses, schema sanity
     (EventName/EventCode present, mapfile regexes compile), license
     headers pass the legal pass of A17.
  4. Replace `external/pmu-events/`, byte-exact upstream files.
  5. Rewrite `RECORD`: kernel ref + tag, date, fetch URL, per-file
     sha256 manifest, exclusion list.
  6. Bump the gate constant (same TU pattern as `zlib_gate.cpp`).
  7. Print a change digest: per-arch event counts old → new, added/
     removed arch dirs, so the reviewer sees churn scale at a glance.

- `--check` (CI): vendored tree hashes match `RECORD`, gate constant
  matches ref. Drift = exit 1. Runs in CI like `prose_gate.py`.

Design calls:
- **Byte-exact vendoring, no normalization.** Review diff *is* the
  upstream diff; manifest gives integrity. Normalized-canonical form
  rejected: extra machinery, destroys upstream diffability.
- No git metadata consulted at build time (yaml-cpp precedent: the
  gate reads recorded constants, not `.git`).
- Upgrade is human-triggered, output committed as any re-pin; CI
  never fetches.
- Fixtures: tiny synthetic table tree under `--check`/strip, wired to
  ctest like `prose_gate_fixtures`.

### A22 — Provider abstraction: the system counts stuff — DECIDED 2026-09-25 (owner directive, organizing principle)
PMU and clocks are **implementation details**, not concepts. The
abstraction: *a system counts things*. A source may be a PMU, a
clock, a push counter, or "a giraffe attached to the system that
honks a horn — I'm counting the honks". Nothing in the core model may
know or care which.

Consequences — the spec is organized around this interface:

- **Core abstraction: the provider contract.** A count provider
  registers named entries into the system catalog; per measurement
  window it yields `raw_delta (uint64) + unit/dimension + per-entry
  metadata (description, availability, caveats like multiplex
  ratios)`. That is the whole contract a provider signs.
- Core vocabulary — catalog, algebra, dimensions, scope, snapshot,
  `.metric()`, provenance — contains **no** `perf_event`, no
  `clock_gettime`, no PMU concept. Those live exclusively inside
  provider implementations.
- Built-in providers are *examples* proving the interface, not the
  design center: `clock` (monotonic, thread/process CPU),
  `linux_pmu` (vendored tables ⊕ sysfs ⊕ probe — all of A16/A17
  machinery confined here), `push` (user hot-path increments),
  `fake` (deterministic provider powering the test suite; A7 falls
  out of the abstraction for free, not as a bolt-on).
- Extensibility: a new source = catalog entry (name, description,
  unit→dimension) + window implementation. The giraffe is a
  100-line provider and a README example; it is not a spec revision.
- Availability stays per-provider, per-entry (A16 two-phase): the
  giraffe may be described-and-countable, described-but-horn-stuck,
  or absent. User code branches on catalog state only — never on
  `#ifdef`, never on provider identity.
- Re-reads earlier questions: "does 007 include clocks?" dissolves.
  007's deliverable is the abstraction, *proven* by shipping
  structurally different providers through one interface —
  time-from-kernel, events-from-hardware, increments-from-user,
  determinism-from-test.

### A23 — Countable objects: the system returns *things* that have counters — DECIDED 2026-09-25 (owner directive)
Counters attach to **objects**. The system returns named objects that
have counters: the uncore, a core, a CPU package, an IOMMU, a
memory controller — and the giraffe. A counter is never free-floating
global truth; it belongs to *a thing*, and the thing has a name.

Structure:
- **Object = named, described, typed node** in a system tree: kind
  (`core`, `package`, `uncore_imc`, `iommu`, `menagerie`...), name,
  description, parent, and its own counter catalog.
- **Every counter belongs to exactly one object.** Clocks attach to
  the root/machine object (`sys.counters()` is the root object's
  catalog, so earlier sketches survive unchanged). PMU events attach
  to the objects whose hardware they measure: core-PMU events to
  cores, DRAM controllers to their uncore instances.
- Platform naming maps in honestly: sysfs PMU device instances
  (`cpu`, `uncore_imc_0`, `ibs_op`, `amd_iommu_0`) are already
  per-instance devices — the PMU provider instantiates them as
  objects. Core objects fan out per-CPU with the kernel's own
  cpu-id naming. hwloc (A22 "later") becomes an *object-tree
  provider*: its topology tree is literally named countable objects,
  and its cpu masks give the PMU targeting for free.
- **Scope measures `(object, expression)` pairs.** Same expression
  measured across many objects — IPC per core, bandwidth per memory
  controller — is fan-out registration: one group per (PMU-instance,
  object) laid out at construction; one window; per-object snapshots.
- **Cross-object composition remains legal** (the scope guarantees
  the shared window): `imc_0["dram_read"] / machine["monotonic"]` is
  bytes-per-second for one controller, folding leaves from two
  objects. The algebra doesn't care; the window alignment is what
  makes it meaningful, and scopes provide it.
- Provenance gains the object: `uncore_imc_0.dram_read = 4.1e9
  (LLC fill bandwidth, ratio 1.00)` — object path in every raw
  entry and metric line.
- Giraffe restated: `sys["menagerie/giraffe-2"]["honks"]`. The
  provider registers objects; objects hold counters; nothing else in
  the model changes — which is the test that A22's abstraction is
  load-bearing.

Open detail (A24): naming scheme for object paths — hierarchical
paths (`package-0/core-3`) vs flat platform instance names
(`uncore_imc_0` as-is). See below.

### A24 — Object naming scheme — DECIDED 2026-09-25
Owner call: **(b) canonical, (a) alias.** Structured identity:
kind + index per level (`package-1/core-3`), enabling structured
selection (`sys.objects(kind=core, package=1)`). The platform name
(`uncore_imc_0`, sysfs/hwloc verbatim) is carried as an alias field;
both resolve; the structured path is the canonical spelling in all
APIs, provenance, and output.

### A16 — Linux query knobs (answered 2026-09-25, verified on dev box)
What Linux actually gives us for "named counters + descriptions":

1. **sysfs PMU registry** — `/sys/bus/event_source/devices/<pmu>/`.
   One dir per PMU: `cpu` (core), `software`, `tracepoint`, `kprobe`,
   `uprobe`, `msr`, `breakpoint`, AMD `ibs_fetch`/`ibs_op`/
   `amd_iommu_0`, Intel `uncore_*`/`intel_pt`/`intel_brcm`. Per dir:
   - `type` — PMU id for `perf_event_open`.
   - `format/<field>` — encoding recipe: `event` → `config:0-7,32-35`,
     `umask` → `config:8-15`, `cmask`, `edge`, `inv`. Tells us how to
     build `attr.config`; tells us nothing about meaning.
   - `events/<name>` — **alias name→encoding only**: `instructions` →
     `event=0xc0`. **No descriptions in sysfs.**
   - `caps/`, `cpumask` (uncore), `perf_event_mux_interval_ms`
     (multiplexer slice length).
   sysfs is the authoritative list of *what this kernel exposes*, and
   it is always present. Descriptions are not its job.
2. **perf vendor-events JSON** — the only rich description source
   the OS can carry: `/usr/share/perf-core/vendor-events/<arch>/*.json`
   (shipped by the perf tools package from kernel
   `tools/perf/pmu-events/arch/...`). Fields: `EventName`, `UMask`,
   `EventCode`, `BriefDescription`, `PublicDescription`, `Unit`,
   `Scale`, `PerPkg`, `Threshold`; metric rows add `MetricName`,
   `MetricExpr`, `MetricGroup`. This machine: **absent** (Arch ships
   perf without the JSON), so `perf list` is thin here. Coverage is
   distro-dependent → cannot be the sole source.
3. **Upstream tables** — `intel/perfmon` JSON (Intel, BSD+) and AMD
   PPR-derived tables in perf's amd dirs. Pinnable, gate-able,
   complete. Dev box is AMD, so Intel-perfmon-only bundling would be
   thin on the author's own machine — the bundle must carry AMD too
   or discovery-first is the default posture.
4. **`perf_event_open` probe** — the only truth of *schedulability
   and permission*. Encoding known ≠ countable now. Catalog entries
   get an availability state computed by test-open: `openable` /
   `permission-blocked` / `not-schedulable`.
5. **Permission knobs** — `/proc/sys/kernel/perf_event_paranoid` (2
   on this box: hardware counting needs `CAP_PERFMON` (≥5.8) or
   `CAP_SYS_ADMIN`), Yama `ptrace_scope` for other-process targets,
   cgroup `perf_event` controller.
6. **Capacity / table selection** — CPUID: vendor/family/model
   (pick the arch table), Intel leaf 0x0A (PMU version, counters per
   event class), AMD leaf `0x80000022` (core PMU counters). Tells the
   group-size limit before multiplexing.
7. **Multiplexing truth at read time** — `read_format`
   `time_enabled` vs `time_running`: running/enabled < 1 ⇒ scaled
   estimate. Contract: metric result carries the ratio; composite
   across multiplexed leaves reports it.
8. Peripheral catalogs (same enumerate() shape, later): tracefs
   `/sys/kernel/tracing/events/<sys>/<evt>/` (names + format, no
   descriptions), `msr` PMU format dir.

Design consequence: **two-phase catalog.** Enumerate = names/
descriptions from bundled JSON ∪ sysfs aliases; *availability* =
computed per entry by probe-open and permission read. A counter
"exists" (described) and "is countable now" (probed) are different
predicates; the catalog reports both.

### A5 — Histogram boundary — DECIDED (histograms → 008; in non-goals list)
HdrHistogram_c is vendored. Does 007 record latencies into histograms
(timer output feeds histogram), or is 007 timer+counter only and
histograms are spec 008?

- **Lean:** 007 produces raw duration samples; histogram recording is
  008. Keeps the vendored-HDR wiring out of 007's acceptance criteria.

### A25 — Point vs quantity: the chrono split, compact sample arrays — DECIDED 2026-09-25 (owner directive, refines A19/A20/A21)
Something was off in earlier drafts: "counter value" conflated two
concepts. `std::chrono` is the model: `time_point` (instant) vs
`duration` (quantity between instants); `time_point + time_point`
does not compile, subtraction yields the quantity. We adopt the
identical split:

- **Point:** one leaf's cumulative reading at an instant —
  `uint64`, monotonic. PMU counters, clock ticks, push counters:
  *all leaves are cumulative tick streams*, including the giraffe's
  honks-since-registration. Nothing leaf-level is ever a delta.
- **Delta:** `point[j] − point[i]`, modular at 2^64 (a single
  hardware wrap subtracts out correctly — the wrap contract moves
  here and becomes trivially true). Only deltas carry dimensions;
  only deltas enter the algebra.
- **Metric:** a fold over deltas drawn from a point *sequence*.
  Two points (first→last) is one case; adjacent pairs per loop
  iteration is the time-series case — sampling-based metrics come
  free with the model, they aren't a later feature.

**Compact sample layout (the hot loop):** `count(); code(); count();
code(); count();` becomes

```cpp
auto rec = plan.make_recorder(capacity);  // construction: SoA layout,
                                          // one uint64 slot per leaf,
                                          // fixed capacity, zero alloc later
rec.sample();      // ← hot path: one PMU group read + vDSO reads +
workload();        //   relaxed loads; appends one COLUMN of raw points;
rec.sample();      //   no fold, no metric, no branch into algebra
```

- Recorder buffer is structure-of-arrays per compiled plan: deltas
  computed at fold time with plain vectorizable subtraction across
  point columns.
- **Parsing the array is the metric layer:** `fold(rec, i, j)` for a
  window; `fold_pairs(rec)` for a per-interval series; `fold(rec)` =
  first→last. Composites are unchanged algebra — they are now
  precisely parsers over point sequences.
- **`time_enabled`/`time_running` are just leaves** of every PMU
  object: cumulative, sampled with the group. Multiplex ratio at any
  interval = `delta(running)/delta(enabled)` — falls out of the same
  delta machinery, structurally honest, zero special cases.
- **A21 scope refined:** a scope is a recorder with exactly two
  points. `scope.start()/finish()/metric()` survives as sugar over
  `sample(); work; sample(); fold(rec, 0, 1)`. One semantics, two
  spellings. "Scope owns the snapshot" → "the recorder owns the
  point buffer; composites are folds over it."
- A19 budget refined: `sample()` is now the named critical-path
  operation (one group read + clock reads + loads, append). Fold
  runs at parse time — off the measurement path entirely, potentially
  much later, even in a different process if point buffers get
  serialized (future report layer).
- A20 survives: raw access = read point columns directly;
  `.metric()` = fold on demand. The struct-with-ratio return now
  *computes* the ratio from enabled/running deltas rather than
  carrying it.
- Contract updates: fold requires `i < j` within recorded extent
  (tier 3); buffer capacity fixed at construction, overflow policy
  open (A26); monotonic-modular invariant per leaf checked at fold:
  for cumulative leaves `delta(p[i], p[i+1])` via unsigned wrap —
  a *decrement* of a push counter between points remains tier 3.

### A29 — Near-single-instruction sample: fast read modes — DECIDED 2026-09-25 (owner directive; supersedes A19/A25 budgets and the A2 TSC exclusion for fast-mode leaves)
Owner expectation: the critical-path counter query must be *near
single instruction* — RDPMC, RDTSC, and equivalents — wherever the
platform permits. The prior µs-class `read(2)` budget is demoted to
the fallback, not the norm.

**Per-leaf read mode, chosen at plan compile, recorded, disclosed:**
- `fast_tsc`: `rdtsc`/`rdtscp` (~20–30 cycles, no syscall) — the
  fast clock leaf. Calibration: nominal/actual frequency from sysfs
  `tsc_khz` + CPUID invariance check at system-open; scaled-TSC
  machines flagged honestly (A22 capability-report style). This
  **reverses the A2 "TSC excluded"** call: TSC is now a first-class
  clock leaf precisely because the hot path needs it.
- `fast_rdpmc`: `perf_event_open` + single-page `mmap` exposes the
  counter for userspace `rdpmc` (~20–30 cycles) — RDPMC returns the
  cumulative value, i.e. exactly a *point* (A25): no model change,
  the fastest thing yet fits the ontology.
  Constraints, probed not assumed: kernel support (Intel ~5.15
  series, AMD later — verify per-kernel during planning, do not
  trust folklore); `kernel.perf_user_access` sysctl (Intel default
  off — TSX side-channel history); pinning/index constraints;
  a multiplexed event's rdpmc value is stale while off-CPU —
  honesty preserved via the mmap page's `timeenable/timerunning`
  (still leaves; folds still compute the ratio; `scaled` still
  structural).
- `syscall`: fallback — group `read(2)` (PMU), vDSO (clocks). Same
  points, same folds; only cadence budget differs.
- `push`: relaxed atomic load (already in-instruction).

**Honest budget table (measured, not asserted — acceptance):**

| mode | per-leaf cost |
|---|---|
| `fast_tsc`, `fast_rdpmc` | ~tens of cycles |
| push load | ~cycles |
| vDSO clock | ~20–25 ns |
| syscall group | ~0.5–2 µs |

A29 supersedes the A19 "one group read per boundary" framing for
fast leaves: a fast plan's `sample()` is a short sequence of
in-instruction reads + pushes into the SoA column — the
`count();code();count()` loop now means what it visually implies.
Tight per-iteration PMU sampling becomes viable (the A28 cadence
warning applies only to syscall-mode plans).

Availability model unchanged but enriched: a counter may be
countable-but-slow (syscall-only) or countable-fast; the catalog
reports the mode per entry after probe; plans compile against the
achieved mode; CI honesty at `paranoid=2` unchanged (fast mode
needs access too — fake/clock carry the suite).

**Reference implementation (owner directive): andikleen/pmu-tools
`jevents/rdpmc.{c,h}` — the canonical ring-3 mmap-page read.**
Verified from source 2026-09-25 (master). Licence: BSD-style Intel
permit-with-attribution — BSD-3-compatible; attribution required
for any code reuse (unlike A17 data, this prior art is legally
touchable). Protocol to mirror:
- `perf_event_open(attr, 0, -1, leader_fd_or_-1, 0)`: per-thread;
  `exclude_kernel=1`, `sample_type=PERF_SAMPLE_READ`; group members
  open with the leader's fd.
- `mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, fd, 0)` — one
  read-only page per fd.
- **The read is a seqcount protocol, not a bare `rdpmc`:**
  `seq = buf->lock; rmb(); index = buf->index; offset = buf->offset;`
  `index == 0` ⇒ rdpmc not allowed (event not pinned / off-CPU) ⇒
  fall back honestly; else `val = _rdpmc(index - 1)` (kernel index
  is 1-based); `rmb()`; retry `while (buf->lock != seq)`. Result
  `(val + offset) & 0xffffffffffff` — kernel `offset` adjustment
  plus 48-bit counter-width mask.
- Modern-kernel correctness beyond jevents' era: also require
  `buf->cap_user_rdpmc` before using the index.
- **Per-thread contexts, same-thread reads only** (jevents doc
  comment: new thread ⇒ new context). Binds to the A28 threading
  contract: plans/recorders are per-thread — this is the kernel
  rule underneath it.
- Ancillary prior art in-tree: `jevents/measure.{c,h}` (measurement
  loop patterns), `examples/rtest{,2,3}.c` (rdpmc usage tests —
  good seeds for our fast-mode acceptance tests), `jestat.c`
  (perf-stat clone: group open/read orchestration).
- Kernel floor for the mmap-page mechanism itself: 3.3+ (jevents
  doc), with the `perf_user_access`/version caveats already listed
  above — still "verify per-kernel during planning, not folklore".

### A26 — Recorder overflow policy — DECIDED 2026-09-25 (both, as compile-time policy)
Owner call: ship both behaviors, selected by **policy type parameter**
at recorder construction — not a runtime flag. `recorder<plan,
hard_stop>` vs `recorder<plan, ring>`: one sample-path implementation
per instantiation, no branch ladder; hot path stays branch-minimal by
construction.

- `hard_stop` (default): overrun is a tier-3 violation. Bounds check
  uses `SG_REQUIRE_ALWAYS`, **not** the semantically-gated default —
  under `ignore` (release) semantics a vanishing bounds check is a
  buffer overflow, and memory safety is never semantic-gated (spec
  001's own vocabulary: `SG_*_ALWAYS` enforces in every
  configuration). Cost: one always-there, perfectly-predicted
  branch beside a ~40 ns clock read — noise, kept deliberately.
- `ring`: index = `idx & (cap−1)` (branchless, cap must be a power
  of two — enforced at construction); a one-shot promotion branch
  (`if unlikely(idx == cap)`) flips the buffer to wrapped state
  once, then settles; wrapped state + dropped count are recorded.
  Folds over a wrapped ring **must** consult `dropped` (parse-time,
  off the hot path) or the metric silently lies — that check lives
  in the fold layer, structurally, like the multiplex ratio.
- Grow-on-demand: still rejected — allocation on the read path is
  the one inviolable rule (A19).
- Rationale for policy-type over runtime config: a runtime flag is
  a branch per `sample()` forever; the type is a branch zero times,
  chosen where slow is allowed (construction). C++23 concept:
  `overflow_policy` with exactly these two models in 007; ring is
  opt-in, never the default — benchmarks know their budgets.

### A27 — Recorder construction syntax — DECIDED 2026-09-25 (exploration)
No visible templates at call sites. Policy passes as a constexpr
**tag value** through a single factory; CTAD selects the
instantiation; codegen identical to explicit template args:

```cpp
auto a = plan.recorder(n_iters + 1);       // hard_stop default
auto b = plan.recorder(1024, sg::ring);    // tag argument, no <>
```

- `recorder_opts` designated-initializer config (`.cap`,
  `.on_full`, CTAD on the policy member) is the documented upgrade
  path when a second knob genuinely appears — not before (A19
  YAGNI discipline).
- Separate factory names per policy rejected: combinatorial naming
  under future options.
- **Recorder is a value handle**: the SoA point buffer lives in a
  plan-allocated arena (allocated at construction, off the crit
  path); `recorder` itself is pointer + index, trivially copyable,
  returned by value. No reference semantics, no lifetime puzzle.
  Multiple recorders over one plan = independent buffers from the
  arena, shared compiled layout.

### A6 — Contract surface — DECIDED 2026-09-25 (table adopted as recommended)
Enforcement tiers: (1) compile-time types, (2) recoverable error,
(3) contract violation (`SG_REQUIRE`, terminate in dev/CI).

- Tier 1: dimension-checked formulas (A10); `.metric()` returns
  `{value, running_ratio, scaled}` struct (A20); same-window
  construction is inherent to the scope API.
- Tier 2 (input failures, program continues): catalog name
  non-resolution (did-you-mean diagnostic), unknown object/kind,
  duplicate counter name within an object, duplicate object name
  under a parent.
- Tier 3 (programmer errors, terminate in dev/CI): `metric()` before
  `finish()`; `finish()` without `start()`; double `start()`;
  measure-after-finish; registering a composite into a started
  scope; negative push increment; provider-internal negative window
  delta (PMU provider additionally handles 64-bit hardware wrap
  correctly).
- `SG_INVARIANT` on every snapshot: all leaves opened/closed in one
  window. The central promise, checked.
- Read-path no-malloc/no-lock/`noexcept`: proven by tests and
  benchmarks (constitution VII), not runtime checks.

### A7 — Testability — RESOLVED by A22
The `fake` provider (deterministic, hand-driven counts) is a first
citizen of the provider abstraction, not a bolt-on fake clock. All
scope/snapshot/algebra/dimension/provenance logic tests against
fakes — no sleeps, no flakes, no privileges. Real-clock tolerance-
band tests remain for the clock provider itself.

## Decisions log

- 2026-09-25 — A8 adopted: unified counter model. Timer is a counter
  over clock ticks. Composite counters (ratio/sum/scale over
  same-window deltas) are first-class. Central type is the
  measurement scope, not the individual timer. Supersedes the
  timer/counter split in A3/A4 framing (their mechanics still apply
  per leaf kind).
- 2026-09-25 — A13 adopted: `system` abstraction with catalog-driven,
  named, described counters (pmu-tools inspiration); catalog is the
  capability report; platform honesty = shorter catalog, not lies.
- 2026-09-25 — A14 decided: C++ algebra front-end in 007; strings
  deferred to 008 with perf-MetricExpr-subset grammar pinned.
- 2026-09-25 — A19 adopted: construction never on critical path,
  reads always; flat compiled read plan; one group read per scope
  boundary; no malloc/lock/lookup in read path.
- 2026-09-25 — A20 adopted: raw leaf access + provenance from every
  composite; metric fold lazy, on `.metric()`, over stored deltas;
  metric result carries multiplex ratio. Resolves A11 value model.
- 2026-09-25 — A17 proposed / A18 decided: rich layer vendored from
  kernel `tools/perf/pmu-events` (path snapshot + gate), byte-exact,
  with `tools/pmu_events/` upgrade + `--check` utility. Linux-only
  focus per owner; hwloc et al. enter `system` later.
- 2026-09-25 — A21 decided (Option A): scope owns the snapshot;
  composites fold; single-composite begin/end is sugar over a
  one-composite scope.
- 2026-09-25 — A10 decided (option a): two compile-time dimensions
  (time, events), construction-side checking, runtime erasure;
  catalog Unit → tag closed switch with unknown-unit resolution
  error.
- 2026-09-25 — A22 adopted as organizing principle: provider
  abstraction. The system counts stuff; PMU/clock/push/fake are
  interchangeable providers behind one contract. Core model is
  provider-agnostic.
- 2026-09-25 — A23 adopted: counters attach to named countable
  objects (uncore, core, package, giraffe); scopes measure
  (object, expression) pairs with fan-out; cross-object composition
  legal under one scope window.
- 2026-09-25 — A24 decided: structured paths canonical
  (`package-1/core-3`), platform names as resolvable aliases.
- 2026-09-25 — A6 adopted as recommended: three-tier enforcement
  (types / recoverable errors / contract violations), snapshot
  carries the same-window `SG_INVARIANT`.
- 2026-09-25 — A7 resolved by A22: fake provider is the test spine.
- 2026-09-25 — Discussion closed. Scope statement below is the
  `/speckit.specify` input.
- 2026-09-25 — A28 gbench suitability: suitable; threading and
  cadence contracts added to 007; harness mapping notes fenced to
  008. Mermaid confirmed as house diagram convention (all six
  prior plans; constitution names views, not tools).
- 2026-09-25 — Final review pass: Momus gate returned REJECT on
  form only (journal is spec input, not a work plan — plan review
  belongs at `/speckit.plan` output in `.omo/plans/`); its
  reference check confirmed all repo claims. Own pass found and
  fixed three content defects: provider contract restated in
  point/delta terms (was pre-A25 "yields delta"); PMU group-read
  budget corrected to per-PMU (groups don't span PMU boundaries);
  snapshot invariant restated per-sample-column per-plan-binding.
  Journal closed for handoff to `/speckit.specify`.
- 2026-09-25 — A29 (owner directive): critical-path reads must be
  near single instruction — per-leaf read modes `fast_tsc` (rdtsc,
  reversing the A2 TSC exclusion), `fast_rdpmc` (mmap'd perf page +
  userspace rdpmc, probed: kernel version, `perf_user_access`,
  pinning), `syscall` fallback, push-load. Plans compile against
  achieved mode; catalog discloses it; acceptance benchmarks both
  budgets. Scope statement amended (principle 6, clock provider,
  PMU provider, non-goals TSC reversal, acceptance fast-mode test).
- 2026-09-25 — POST-CLOSURE A25: chrono split adopted — point
  (cumulative snapshot) vs delta (quantity); all leaves are
  cumulative streams; metrics are folds over point sequences;
  compact SoA recorder for `count();code();count()` hot loops;
  enabled/running become leaves (ratio = delta quotient); scope =
  2-point recorder sugar. Refines A19/A20/A21. Scope statement
  amended below.
- 2026-09-25 — POST-CLOSURE A26: overflow = compile-time policy
  type (`hard_stop` default with `SG_REQUIRE_ALWAYS` bounds; `ring`
  opt-in, power-of-two, dropped-count mandatory at fold). Runtime
  mode flags rejected (branch per sample). Scope statement
  amended; all axes now closed.
- 2026-09-25 — POST-CLOSURE A27: call sites carry no templates —
  constexpr tag value through one factory (`plan.recorder(cap,
  sg::ring)`), CTAD under the hood; recorder is a value handle
  into a plan arena; designated-init config struct is the
  documented later upgrade path.

## Resolved scope statement — INPUT TO `/speckit.specify`

*(This section is self-contained: it is the feature description to
hand to `/speckit.specify` for `specs/007-counters-and-timers`. The
axes above are the discussion record; nothing here requires reading
them.)*

### Feature

A **standalone counters library**: the system is modeled as named,
countable objects; every object has a catalog of named, described
counters; counters compose with C++ arithmetic into metrics; a
recorder samples points in the hot path and composites fold them,
with full provenance.

**Standalone is the product.** 007 ships classes, not a harness:
zero dependency on any benchmarking framework, usable directly in
ordinary C++ code — manual loops, daemons, services, other
frameworks. The speedgun benchmark harness is a future spec built
*on top of* these classes; 007 must not anticipate it and must not
preclude it.

**Direct embeddability into Google-Benchmark-style iteration loops
is an explicit design goal** (reviewed in A28): construction fits
gbench's untimed setup region, `state.max_iterations` sizes the
recorder, per-thread plans serve `->Threads(n)`, folds feed
`state.counters`. 007 itself contains **no gbench code** — the
glue/harness is future intent; what is binding here is that the
class design stays directly embeddable.

The name says "counters and timers" because a timer is not a separate
concept: **a timer is a counter whose events are clock ticks.** No
timer/counter split exists in the model.

### Organizing principles (binding)

1. **Provider abstraction.** "The system counts stuff." Providers
   register named counters; each `sample()` yields for every leaf
   under management a cumulative `uint64` **point** + unit +
   metadata (description, availability, caveats) — deltas are
   fold-time arithmetic, not a provider concept (see 5). The core
   vocabulary contains no `perf_event`, no `clock_gettime`, no PMU
   concept — those live only inside provider implementations. A
   counter source may be hardware PMU, the kernel's clock, user
   code increments, or a giraffe honking a horn; the model must not
   care.
2. **Countable objects.** Counters belong to named objects in a
   tree: `machine`, `package-1/core-3`, `uncore_imc_0`,
   `amd_iommu_0`. Identity is the structured path (canonical,
   selectable: `sys.objects(kind=core, package=1)`); the platform
   instance name is a resolvable alias. Every counter attaches to
   exactly one object; clocks attach to the machine object.
3. **Catalog as capability report.** Enumerate what exists; never
   lie. Two-phase truth per entry: *described* (from data) vs
   *countable-now* (probed, permission-aware). User code branches
   on catalog state only.
4. **Composition is algebra.** `sys["instructions"] /
    sys["cycles"]`, `bytes / monotonic` — operators over resolved
    counters, operating on *deltas between points* (see 5).
    Compile-time dimension system `time^t × events^c`:
    `+` requires identical tags, `/` subtracts exponents; violations
    do not compile. Tags are construction-time only and erased at
    read time.
5. **Point vs quantity (the chrono split).** A leaf reading is a
    *point*: cumulative `uint64` at an instant — PMU counters,
    clock ticks, push counters, all one kind (the giraffe counts
    honks-since-registration). A *delta* is `point[j] − point[i]`,
    modular at 2^64, so a single hardware wrap subtracts out. Only
    deltas carry dimensions or enter the algebra. A metric is a
    fold over deltas drawn from a point sequence: two points is
    the window case, adjacent pairs is the time-series case —
    sampling series are in the model from day one, not a feature.
6. **Construction/read split.** Construction (name resolution,
    algebra, dimension check, PMU group layout, plan compilation,
    SoA buffer layout, allocation) is never on the critical path.
     The critical-path operation is `recorder.sample()`. Where the
     platform permits, leaf reads are **near single instruction**:
     `rdtsc`/`rdtscp` for the fast time leaf, `rdpmc` against
     mmap'd perf pages for pinned PMU leaves (~tens of cycles).
     Per-leaf read mode — `fast_tsc` / `fast_rdpmc` / `syscall`
     (group `read(2)` per PMU leader, vDSO clocks; groups do not
     span PMU boundaries) / push-load — is probed, chosen at plan
     compile, recorded, and disclosed in the catalog. A fast plan's
     `sample()` is a short in-instruction sequence pushing one
     column into the SoA buffer: no malloc, no lock, `noexcept`;
     `count(); code(); count();` in a tight loop is the design
     center and means what it implies. Folds run later, off the
     measurement path, over the recorded buffer.
7. **The recorder owns the buffer; composites are parsers.** A
    composite folds point sequences: `fold(rec, i, j)` for a
    window, `fold_pairs(rec)` for per-interval series. A scope is
    exactly a two-point recorder; `start()/finish()/metric()` is
    sugar over `sample(); work; sample(); fold(rec, 0, 1)` — one
    semantics, two spellings. Fold output is
    `{value, running_ratio, scaled}` with the ratio *computed*
    from the enabled/running deltas — multiplex disclosure stays
    structurally impossible to drop. Every composite exposes raw
    point columns with provenance: `package-1/core-3.instructions
    = 1.2e10 @ point 4 (retired instructions, cumulative)`.

### Providers shipped as examples (proving the interface)

- `clock`: monotonic wall, thread CPU, process CPU (no privileges;
  dimension `time^1`) — plus the fast `tsc` leaf (`rdtsc`-grade,
  frequency calibrated at system-open from sysfs/CPUID, scaled-TSC
  machines flagged honestly).
- `push`: user hot-path increments (`add(n)`, relaxed atomic).
- `fake`: deterministic, hand-driven; the test spine for all
  scope/algebra/provenance logic — no sleeps, no flakes, no root.
- `linux_pmu`: the rich backend (below). Linux-only; other platforms
  simply have shorter catalogs — no API difference.

### Linux PMU provider: rich catalog, vendored and gated

- Catalog data vendored from the kernel's `tools/perf/pmu-events`
  tree (Intel ⊕ AMD tables + `mapfile.csv` CPUID→table map), path
  snapshot at a pinned commit under `external/pmu-events`,
  byte-exact, version-gated like `zlib_gate.cpp`. Licensing:
  `tools/perf` dual MIT/GPLv2; confirm legal pass during planning.
- Runtime: CPUID picks the table (lazy, parse-once with vendored
  simdjson); JSON `Unit` maps to dimension tags through a closed
  switch (unknown unit = resolution error, never a guess); event
  encoding composes JSON semantic codes with sysfs `format/` bit
  layouts; availability from `perf_event_open` probe +
  `perf_event_paranoid` state.
- **Fast read mode**: where probed available — mmap'd single page
  + userspace `rdpmc` (kernel-version and `perf_user_access`
  sysctl gates verified per-kernel during planning, not folklore;
  pinning/index constraints enforced at plan compile). A
  multiplexed event's fast value is stale while off-CPU; the mmap
  page's `timeenable/timerunning` remain leaves, so the fold's
  `scaled` honesty survives every mode. `syscall` mode remains the
  fallback; catalog reports achieved mode per entry.
- **Fast-mode reference**: `jevents/rdpmc.{c,h}` from
  andikleen/pmu-tools (BSD-style, attribution-compatible) — the
  canonical mmap-page protocol: seqcount lock retry, 1-based
  `index` (0 ⇒ not-allowed, honest fallback), `offset` adjustment,
  48-bit mask, per-thread same-thread contexts, modern
  `cap_user_rdpmc` gate. Implementation mirrors this protocol;
  `examples/rtest*.c` seed the acceptance tests. A29 carries the
  full notes.
- Upgrade utility `tools/pmu_events/update_pmu_events.py`: `--to
  <kernel-ref>` fetch (cgit path archive), strip (explicit
  exclusion list), validate, replace, rewrite `RECORD` (ref, URL,
  per-file sha256, exclusions), bump gate, print per-arch change
  digest; `--check` verifies manifest + gate in CI, never fetches.
  Ctest fixtures like `prose_gate_fixtures`.

### Contract enforcement (constitution II; three tiers)

- Types (never compiles): dimension violations; metric-result
  struct; same-window construction.
- Recoverable errors: name/object non-resolution (did-you-mean),
  duplicate counter name in an object, duplicate object name under
  a parent.
- Contract violations (`SG_REQUIRE`, dev/CI terminate): `metric()`
  before `finish()` (scope sugar); registering a composite into a
  started scope; `sample()` past capacity under `hard_stop`; push
  counter decrement between points; provider-internal non-modular
  go-backwards. Hardware wrap is not a violation — modular delta
  handles it (see 5).
- `SG_INVARIANT` per sample column: all leaf values in a column
  were read within one sampling action under one plan binding;
  folds consume columns of a single recorder only.
- Fold range validity (`i < j` within recorded extent) and
  per-leaf monotonic-modular checks: tier 3. Recorder capacity is
  fixed at construction; overflow is a compile-time policy type:
  `hard_stop` (default; `SG_REQUIRE_ALWAYS` bounds — memory safety
  is never semantic-gated) or `ring` (power-of-two mask,
  branchless, wrapped-state recorded; folds must consult
  `dropped`). No grow-on-demand, no runtime mode flag.
- Read-path budget proven by benchmark tests, not runtime checks.

### Non-goals / deferred (explicit)

- Histograms and latency distributions → 008 (HdrHistogram_c stays
  untouched by 007).
- String-formula metrics and ingestion of bundled `MetricExpr`
  rows → 008; grammar is perf-MetricExpr-subset, never a second
  dialect.
- ~~TSC/cycle-granularity leaves~~ — REVERSED by A29: the `tsc`
  fast leaf is IN (rdtsc-grade sampling clock, frequency calibration
  included). Still deferred: full TSC-based *wall-clock conversion*
  machinery beyond what the leaf needs.
- hwloc-fed
  object trees, report serialization (simdjson/yaml output),
  benchmark harness/registration API, multiplexing-aware group
  scheduling heuristics, Windows/macOS PMU providers.
  Each is 008+ material; none may leak into 007 acceptance.

### Acceptance must include

- Standalone proof: an example target using only the public headers
  + std — no benchmarking framework, no third-party deps — compiles,
  runs, and folds a metric; link manifest shows only speedgun-ng.
- Giraffe test: an out-of-tree provider (100-line example) registers
  objects + counters and composes through the unchanged core — the
  abstraction's proof.
- Privilege-free CI: full suite green with `perf_event_paranoid=2`
  via fake + clock providers; PMU tests assert honest
  `permission-blocked` states instead of failing.
- Read-path benchmark: clock-only `sample()` within stated ns
  budget; PMU group read cost stated per plan; overhead calibration
  exposed per composite plan; `sample()` does zero allocation
  (asserted, e.g. via counting allocator or `noexcept` + malloc
  interposition).
- Fast-mode benchmark, measured not asserted: on a fast-capable
  host (probe-gated test, honest skip elsewhere) a `fast_tsc` +
  `fast_rdpmc` plan's `sample()` lands in the tens-of-cycles
  regime, and the same plan in `syscall` mode lands in the
  µs-regime — the two budgets documented side by side.
- Cross-object fan-out: IPC for all cores, one window, one group
  read per core, values reconcile against shared `instructions`
  deltas.
- Point/delta round-trip: a hand-built point buffer folds to exact
  expected metrics for a known fake-provider sequence, including a
  crafted 2^64 wrap on a PMU-style leaf (delta subtracts out) and a
  `fold_pairs` time series.
- Hot-loop idiom compiles to the contract: `count(); code(); count()`
  over a fixed-capacity recorder, sampled in a benchmark, shows fold
  cost entirely absent from the sample path.

## A28 — Google Benchmark suitability review — CLOSED 2026-09-25

Reviewing 007 as standalone classes that a gbench function uses
directly (harness itself remains 008). Verdict: suitable. gbench's
execution shape is the point model's home ground: pre-loop setup is
untimed (A19 aligns with gbench's timing boundary);
`state.max_iterations` makes recorder capacity computable at setup;
`pauseClock/resume` is a degenerate case of fold-over-any-two-points;
provenance paths become `state.counters` keys.

### Class inventory (the 007 surface)

```mermaid
classDiagram
  direction LR
  class provider { <<concept>>
    +enumerate() objects+entries
    +open(entry) window reader }
  class system { +local() system&
    +object(path) object&
    +objects(kind, filters) range
    +register_provider(p) }
  class object { +path canonical, alias platform
    +counters() catalog
    +children() range }
  class catalog_entry { +name, description
    +unit → dim
    +availability }
  class "counter<D>" as counter { <<resolved leaf>> }
  class "expression<D>" as expression {
    +operator+ operator/ scalar_scale
    +fold(rec, i, j) metric_result
    +fold_pairs(rec) series
    +raw(object, leaf) point columns }
  class plan { <<compiled: slots, group layout, arena>>
    +recorder(cap, policy) recorder<P> }
  class "recorder<P>" as recorder { <<value handle>>
    +sample() noexcept
    +points() buffer }
  class metric_result { +value, running_ratio, scaled }
  system "1" *-- "*" object
  object "1" *-- "*" catalog_entry
  catalog_entry ..> counter : resolve
  expression o-- counter : leaves
  plan ..> expression : compile
  recorder --> plan : arena buffer
  expression ..> recorder : fold
  provider <|.. clock_provider
  provider <|.. push_provider
  provider <|.. fake_provider
  provider <|.. linux_pmu_provider
```

### Skeleton (signatures, 007-level only)

```cpp
namespace sg {
  template<int T, int C> struct dim {};                    // time^T events^C
  enum class availability { countable, permission_blocked,
                            not_encodable, absent };
  struct catalog_entry { std::string_view name, description;
                         unit unit; availability avail; };
  struct metric_result { double value, running_ratio; bool scaled; };

  struct provider {          // concept, spelled as above
    // enumerate() -> objects+entries ; open(entry, target) -> window reader
  };

  class system {
   public:
    static system& local();                                  // process singleton
    object& object(std::string_view path);                   // canonical or alias
    object_range objects(std::string_view kind);             // structured select
    void register_provider(std::unique_ptr<provider>);       // pre-open only
  };

  template<dim D> class counter;                             // resolved leaf
  template<dim D> class expression {
   public:
    metric_result fold(const recorder_api&, size_t i, size_t j) const;
    std::vector<metric_result> fold_pairs(const recorder_api&) const;
    points_view raw(std::string_view object_path) const;     // provenance
  };

  class plan {
   public:
    template<overflow_policy P = hard_stop>
    auto recorder(size_t capacity) const -> recorder_handle<P>;
  };
  auto compile(system&, /* expression leaves... */) -> plan; // group layout etc.

  template<overflow_policy P> struct recorder_handle {       // value handle
    void sample() noexcept;                                  // THE hot path
  };
}
```

### Threading contract (gap fix, binding on 007)

- `system` and all catalogs: immutable after open; concurrent reads
  are safe by construction.
- **Plans and recorders are per-thread objects.** PMU leaf targeting
  (thread or cpu, `perf_event_open` pid/cpu args) binds at
  plan-open. gbench `->Threads(n)`: each thread constructs its own
  plan (setup, untimed) + recorder; folds are per-thread at the end.
- No plan may assume a single global instance; multiple plans per
  system are first-class.

### Cadence contract (gap fix, binding on 007 docs)

- Sampling cadence is caller-owned. The documented idiom for tight
  loops is chunked: sample every K iterations, capacity
  `N/K + 1`, `fold_pairs` yields per-window metrics (K=1 =
  per-iteration, with its observer-effect cost stated numerically
  from the overhead calibration).
- A fold window includes the sample cost of its endpoints — stated,
  calibrated (A3), not hidden.

### Harness seam notes (008, recorded so the mapping survives)

- gbench divides `state.counters` by iterations unless
  `kAvoidDivideByIteration`. Rule: ratio metrics (IPC) and rates
  (bytes/sec) register AVOID_DIVIDE; only per-iteration counts
  (bytes processed) use defaults. Getting this wrong silently
  divides correct numbers into nonsense.
- `state.SkipWithCrash` / workload exceptions: all construction
  resources (fds, arenas) are RAII, unwind-clean — acceptance item.
- gbench's own wall-clock and the monotonic leaf are independent
  measurements of the same window; agreement within calibration is
  an integration test for 008, not a 007 promise.
