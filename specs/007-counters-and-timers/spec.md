# Feature Specification: Standalone Counters Library (Counters and Timers)

**Feature Branch**: `007-counters-and-timers`

**Created**: 2026-09-25

**Status**: Draft

**Input**: User description: the resolved scope statement of `sg_counters.md` (the closed discussion journal for this feature). A standalone counters library: the system is modeled as named, countable objects; every object has a catalog of named, described counters; counters compose with C++ arithmetic into metrics; a recorder samples points in the hot path and composites fold them, with full provenance. A timer is a counter whose events are clock ticks; the model has one concept. 007 ships classes with zero dependency on any benchmarking framework, directly embeddable in ordinary C++ iteration loops; the speedgun harness is a future spec built on top of these classes.

## Overview

The library answers one question in every shape: *how many of what happened during this window?* Sources are interchangeable behind a single provider contract: kernel clocks, hardware performance counters, user hot-path increments, or an out-of-tree source the library has never heard of. Counters attach to named objects in a system tree (`machine`, `package-1/core-3`, `uncore_imc_0`). Counter values combine with arithmetic (`sys["instructions"] / sys["cycles"]`) under a compile-time dimension system. Measurement runs in two phases with a hard boundary: everything slow (name resolution, algebra checking, hardware grouping, memory allocation) happens at construction; the critical-path operation `sample()` appends one column of cumulative raw points and does nothing else. Derived metrics are computed later, by folding recorded point sequences, off the measurement path, as often as the caller asks.

Two correctness promises run through the whole design:

1. **One window, one truth.** All leaves of a compiled plan are read within a single sampling action, so every composite folded from the same recorder sees identical deltas. Cross-metric agreement is structural.
2. **No number without its provenance.** Every metric result carries the multiplex ratio and a scaled flag; every composite exposes each constituent's raw point column with object path, description, unit, and availability. A derived metric is a view over inspectable parts.

## Clarifications

### Session 2026-09-25

- Q: When you build a recorder with capacity N, does N count stored point columns or measured intervals? (FR-025) → A: N counts stored point columns; capacity 1 holds exactly one sample, and the minimum capacity for any fold is 2.
- Q: What running-ratio and scaled values must a fold report when its sources have no enabled/running pair, such as `bytes / monotonic` over push and clock counters? (FR-019) → A: Sources without enabled/running leaves disclose ratio 1.0 and scaled false; a composite's ratio is the product of constituent ratios each raised to its algebraic exponent.
- Q: May several threads call `add()` on the same push counter at the same time, and what should a sample see? (FR-035, FR-050) → A: Single-threaded access only; the counter is confined to its owning thread, cross-thread access is a contract violation, and increment plus read are plain non-atomic operations for minimal overhead; threading policy is left to a future spec built on top.

### Session 2026-09-27

- Q: SC-004 and T051 ask for one plan measured in both read modes with a fixed factor between the medians, while `counters_overhead` measures two one-leaf plans and the recorded `perf_event_paranoid` 2 host probes a fast mechanism (`docs/pages/counters-overhead.md:296-299`) with no syscall-mode counterpart to compare it against (`docs/pages/counters-overhead.md:256-263`). Which reading governs the criterion? (T080) → A: The two-plan comparison is the criterion the artifact performs, so SC-004 names both plans and carries no fixed factor. A `sample()` action charges the recorder bookkeeping and the contract checks on top of the read, and 100x below either per-action median the release build (`-O3 -DNDEBUG`, contracts `ignore`) records for its reference host, 40 ns for the clock plan and 70 ns for the core-PMU group (`docs/pages/counters-overhead.md:149-150`), is 0.4 ns and 0.7 ns, each below one clock read, so the factor is unreachable for any read mechanism. The pass check survives as an order comparison on a probe-passing host: the fast-mode median is below the syscall-mode median. A forced-`syscall` read-mode override on `compile()` is public surface with one caller, and it lands with the spec that measures one plan in both regimes.
- Q: FR-049 and SC-001 ask for a link manifest of only this library, while `add_library` takes no type keyword and the build produces `libspeedgun-ng.a`, so `ldd` and `readelf -d` name no `speedgun-ng` entry at all. What does the manifest demonstrate? (T120) → A: No third-party dynamic dependency. Both tools name `libstdc++`, `libm`, `libgcc_s` and `libc`, the platform C and C++ runtime, and the standalone example's manifest matches neither `speedgun-ng` nor any vendored dependency.
- Q: T041 says the example composes its metric over fake+clock sources, while the example registered the fake provider alone. Which governs? (T121) → A: Both sources. The example registers the clock provider beside the fake one, and one sampling action reads a fake leaf and a clock leaf together (FR-047), which is what its own header comment claimed.
- Q: quickstart section 1 promises a skipped test names its failure, while CTest prints `***Skipped` for a test whose exit code matches `SKIP_RETURN_CODE` and swallows the captured output. Which mechanism shows the reason? (T122) → A: `ctest -V`; the reason reaches the console only there, and `SKIP_REGULAR_EXPRESSION` leaves the console line unchanged, recording the regex rather than the matched text in `LastTest.log`. Quickstart section 1 now names `-V`.
- Q: FR-024 requires shared target and clock identity validated at construction, while `compile` takes one `target` for a whole plan and `plan_impl` holds one `bound_target` (`source/counters/detail/core.hpp:104`). Which obligation does the library carry? (T086, T147) → A: The single-plan-target design. `compile` at `include/speedgun-ng/counters_measurement.hpp:1089-1095` takes that one target, `compile_core` stores it once at `source/counters/plan.cpp:469`, hands the same value to every provider's `open()` at `source/counters/plan.cpp:501`, and both read modes derive their `(pid, cpu)` from it through `leader_pid` at `source/counters/detail/pmu.hpp:292`. A mismatch across group members is unrepresentable, so the construction errors FR-024 keeps are the two a single target can still fail on: a leaf the catalog reports as not `countable`, whose message names the leaf address and the catalog state (`source/counters/plan.cpp:448-457`), and a window a provider refuses to open, whose single diagnostic reports that one provider's window for its leaves could not be opened and names no leaf, no provider, and no kernel reason (`source/counters/plan.cpp:502-504`), so the caller narrows the refusal by compiling the expression's leaves one at a time. Both land before any hardware read. Per-thread identity is the other half of the binding and stays a contract violation at sample time (`source/counters/plan.cpp:61-62`, FR-031).
- Q: FR-034 calibrates the `tsc` leaf at system-open, while the calibration runs in the `clock_provider` constructor at `source/counters/clock_provider.cpp:206-236`. Which boundary does the requirement name? (T127, T153) → A: Provider construction. `enumerate` is const and runs at registration, registration is refused after open, and the catalog freezes at the open boundary (FR-009), so a calibration deferred to open would publish an uncalibrated leaf. The constructor is the last point at which the leaf, its provenance, and its scaling flag can be seeded, and the seed is what the catalog freezes.
- Q: The scope-misuse edge case lists registering a composite into a started scope as a contract violation, while `scope` exposes `start`, `finish`, `view`, and `metric` and no registration entry point (`source/counters/plan.cpp:312-356`, `include/speedgun-ng/counters_measurement.hpp:981-1053`). Which sequences can the type refuse? (T089, T155) → A: Three. `metric` on a window that is not closed (`source/counters/fold.cpp:289-290`), `finish` without `start` (`source/counters/plan.cpp:344-345`, the same guard refusing a second `finish`), and a second `start` (`source/counters/plan.cpp:330`). A composite reaches a window through the plan compiled before that window opens, and a scope registry would be the only way to register into a running scope, so that misuse has no spelling.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Compose and read a derived metric in one measurement window (Priority: P1)

A developer embedding the library in ordinary C++ code asks the local system for a named counter on an object, composes counters with arithmetic (`instructions / cycles` builds an IPC expression; the dimension system accepts it, and it is a compile error to add a time counter to an event counter), opens a scope, runs work, finishes, and reads the metric. The result is a value plus the multiplex running ratio plus a scaled flag. Asking again re-folds the stored raw points; nothing re-reads hardware. The raw constituent columns are inspectable with full provenance: `package-1/core-3.instructions = 1.2e10 (retired instructions, cumulative)`. All of this is testable end to end against the shipped deterministic `fake` provider: no sleeps, no flakes, no privileges.

**Why this priority**: This is the core value slice: catalog lookup, name resolution, typed algebra, one-window measurement, lazy fold, and provenance. Everything else in the feature is providers and policy attached to this spine. With only this, a user already measures derived metrics truthfully through a source interface.

**Independent Test**: Build the library, write a program that composes an expression over fake-provider counters, measures it with the scope spelling, and checks the folded value, ratio field, and raw provenance against hand-driven point sequences. All assertions are exact; no timing tolerance is needed.

**Acceptance Scenarios**:

1. **Given** a system with the fake provider registered, **When** the user resolves a described counter by name, **Then** resolution succeeds and the handle carries its name, description, unit dimension, and availability state.
2. **Given** a described name typed with one character wrong, **When** resolution runs, **Then** it fails with a recoverable error whose diagnostic suggests near-miss names and descriptions from the catalog.
3. **Given** two resolved counters with dimensions `events^1`, **When** the user composes `a / b`, **Then** construction succeeds and the expression carries dimension `events^0`; **And** when the user attempts to add a `time^1` counter to an `events^1` counter, **Then** the program fails to compile.
4. **Given** a composed expression and a scope, **When** the user calls `start()`, runs work whose fake counts are hand-driven, calls `finish()`, then calls `.metric()`, **Then** the result equals the hand-computed quotient of the driven deltas and includes the running-ratio and scaled fields.
5. **Given** a finished scope, **When** `.metric()` is called ten more times, **Then** all ten results are identical and the provider records zero additional reads.
6. **Given** a catalog unit string outside the closed mapping (an unrecognized unit in provider-supplied metadata), **When** resolution maps it to a dimension, **Then** resolution fails with an error naming the unit; nothing is guessed.
7. **Given** a scope, **When** the user requests the raw view of a composite's constituent leaf, **Then** the view reports object path, name, description, unit, raw delta, and multiplex ratio for that leaf.

---

### User Story 2 - Sample points in a hot loop and fold them later (Priority: P1)

A developer compiling an expression gets a plan, then a recorder with a fixed capacity chosen at setup. The hot loop is `rec.sample(); code(); rec.sample(); code(); ...`. Each `sample()` appends one column of raw cumulative points and performs no fold, no allocation, and no lock. After the loop the developer folds windows: `fold(rec, i, j)` for one interval, `fold_pairs(rec)` for a per-interval series, `fold(rec)` for first-to-last. Overflow is chosen by a compile-time policy: `hard_stop` (default) turns past-capacity sampling into an immediate contract violation enforced in every build configuration; `ring` overwrites the oldest samples branchlessly, records the dropped count, and forces folds to account for drops. A hardware counter that wraps 2^64 subtracts out correctly inside a delta.

**Why this priority**: The point/delta/recorder model is what makes `count(); code(); count()` cheap while keeping wrap and drop accounting correct. Without it, user 1's window is the only measurement the library can produce. With it, time series and tight-loop measurement exist, still testable end to end with fake providers.

**Independent Test**: Drive a fake provider through a known point sequence including a crafted 2^64 wrap on a PMU-style leaf; assert window folds, pair folds, wrapped-ring folds, and drop accounting produce exactly the hand-computed values. A benchmark target asserts `sample()` performs zero allocations.

**Acceptance Scenarios**:

1. **Given** a compiled plan and capacity N, **When** the user creates a recorder through the factory with the default policy, **Then** the recorder holds N point columns allocated at construction, and later sampling allocates nothing.
2. **Given** a `hard_stop` recorder at capacity, **When** `sample()` is called once past capacity, **Then** the program aborts with a contract violation, in release configuration included.
3. **Given** a `ring` recorder of power-of-two capacity C, **When** sampling exceeds C, **Then** the write index masks branchlessly, the wrapped flag and dropped count are recorded, and a fold over the buffer reports results consistent with the retained window; **And** when the user constructs a ring recorder with a capacity that is not a power of two, **Then** construction fails.
4. **Given** a recorded fake sequence where one PMU-style leaf wraps past 2^64 between two points, **When** any window containing that boundary is folded, **Then** the delta equals the true count (the wrap subtracts out under modular arithmetic).
5. **Given** a recorded sequence of three or more points, **When** `fold_pairs` runs, **Then** each adjacent pair yields its own metric result, giving a per-interval series.
6. **Given** a fold range `[i, j]` where `i >= j` or `j` exceeds the recorded extent, **When** the fold runs, **Then** it is a contract violation (developer error, checked in dev/CI).
7. **Given** the same compiled plan, **When** the user creates two recorders, **Then** each has an independent point buffer and both share the compiled layout.

---

### User Story 3 - Counters belong to named things; measure across the machine (Priority: P2)

Counters never float free: each belongs to exactly one object in a system tree, and the thing has a name. The user selects objects structurally (`sys.objects(kind="core", package=1)`), resolves counters through object paths, and composes across objects: bytes-per-second for one memory controller divides that controller's event column by the machine's clock column, and the shared window makes the cross-object ratio meaningful. Fan-out registration measures one expression across many objects in one window: IPC for every core, one group read per PMU instance, per-object snapshots that reconcile against each other. Canonical spelling is the structured path; platform instance names resolve as aliases; provenance lines carry object paths.

**Why this priority**: The object tree multiplies the value of the core model (per-core, per-controller measurement) but rides entirely on stories 1 and 2 mechanics. It changes where catalogs hang; how measurement works stays identical. Clocks on the machine object already prove the single-object case.

**Independent Test**: Register a fake provider that builds a two-package, multi-core tree with per-core event counters; assert selection, alias resolution, cross-object composition, fan-out window counts, and provenance strings, all with exact values.

**Acceptance Scenarios**:

1. **Given** a provider that registers objects, **When** the user enumerates the tree, **Then** each object reports kind, canonical structured path, description, parent, platform alias where one exists, and its own counter catalog.
2. **Given** the canonical path `package-1/core-3` and the platform alias for the same object, **When** either spelling is resolved, **Then** both resolve to the same object; **And** all API output, provenance, and diagnostics print the canonical path.
3. **Given** a selection `objects(kind=core, package=1)`, **When** it runs, **Then** exactly the matching objects are returned.
4. **Given** an expression mixing `uncore_imc_0`-attached and machine-attached counters, **When** it is measured in one scope, **Then** both leaves are read within one sampling action and the fold succeeds.
5. **Given** one IPC expression registered across every core object, **When** one window runs, **Then** each core gets its own metric result, and the sum of per-core instruction deltas reconciles with the shared total.
6. **Given** a provider registering two objects with the same canonical path under one parent, **When** registration runs, **Then** it fails with a recoverable duplicate-name error; the same applies to duplicate counter names within one object.

---

### User Story 4 - Measure real time and user events with no privileges (Priority: P2)

The shipped `clock` provider puts monotonic wall time, thread CPU time, and process CPU time on the machine object, all with dimension `time^1`, readable with zero privileges. The shipped `push` provider lets user code count its own events from the hot path: the counter is confined to one thread, `add(n)` is a plain non-atomic increment the compiler can keep register-resident, and sample time is a plain load. Composites mix them freely: bytes-per-second is a push byte counter divided by a clock counter. The fast `tsc` leaf adds a rdtsc-grade time source, calibrated at provider construction from platform frequency data, with its achieved read mode disclosed in the catalog.

**Why this priority**: These providers make the library useful in an unprivileged process today and prove the provider contract covers both kernel time and user increments. They are the acceptance vehicle on every platform.

**Independent Test**: Tolerance-band tests over sleep-free busy windows compare clock leaves against each other and against the process's own accounting; push counters fold to exact totals. The bytes-per-second composite folds with exact arithmetic over fake clock columns and real push counts.

**Acceptance Scenarios**:

1. **Given** an unprivileged process, **When** a scope over monotonic, thread-CPU, and process-CPU leaves wraps known work, **Then** all three deltas are positive and fall within the calibration tolerance of each other for CPU-bound work.
2. **Given** a push counter, **When** user code on its owning thread calls `add(1000)` between two samples, **Then** the folded delta equals exactly 1000 and the increment and the sample read are plain non-atomic operations.
3. **Given** `bytes / monotonic` where `bytes` is a push counter, **When** folded over a window, **Then** the result is the byte rate with the same ratio-disclosure structure as any other metric.
4. **Given** the machine catalog, **When** enumerated on any platform, **Then** the clock leaves and push counters appear with descriptions and `countable` availability, and each entry reports its achieved read mode.
5. **Given** a platform whose TSC frequency is calibrated, **When** the `tsc` leaf is used, **Then** its frequency provenance is reported; **And** when the platform reports a scaled TSC, **Then** the catalog flags the leaf accordingly.
6. **Given** a push counter decremented between two recorded points, **When** the fold's monotonicity check runs, **Then** it is a contract violation (fold mechanics from US2; the push-specific check lands with this provider).

---

### User Story 5 - Extend the system with a provider the library has never seen (Priority: P2)

A user writes an out-of-tree provider: a giraffe attached to the system that honks a horn, and the user counts the honks. The provider registers objects and named counters with descriptions and a unit mapping, and implements the per-window point yield. The core model is unchanged: the giraffe's counters appear in the catalog, compose in the algebra, sample in a recorder, and fold to metrics exactly like built-ins. The giraffe example ships as roughly a hundred lines of documented sample code.

**Why this priority**: Extensibility is the load-bearing test that the provider abstraction is real. It is prioritized after the core stories because it asserts their interfaces from outside; a failure here is a redesign signal for the whole feature.

**Independent Test**: Compile the giraffe example against the public headers with no access to library internals; run it: catalog enumeration lists `menagerie/giraffe-2` and its `honks` counter, a scope measures it, `honks / monotonic` folds to a honk rate. The core source tree is unmodified by the example.

**Acceptance Scenarios**:

1. **Given** the giraffe provider compiled against public headers only, **When** it registers with a system, **Then** its objects and counters appear in enumeration with descriptions and availability.
2. **Given** a giraffe honk counter and a clock counter, **When** the user composes a rate metric and measures one window, **Then** the fold yields honks-per-second from one shared sampling action.
3. **Given** the giraffe source, **When** inspected, **Then** it implements only the provider contract and touches no library-internal header.
4. **Given** a source whose hardware is present-but-broken, **When** its provider reports availability, **Then** the catalog distinguishes described-and-countable from described-but-unavailable, and user code branches on catalog state alone.

---

### User Story 6 - Count hardware events on Linux from a rich catalog (Priority: P3)

On Linux, the `linux_pmu` provider turns the machine into a catalog of real hardware events: names, descriptions, and units sourced from vendored per-CPU event tables merged with kernel-discovered aliases, with availability computed by probing. CPU identification selects the right event table; encoding composes table semantics with the running kernel's bit layouts; events the kernel cannot encode are reported as described-but-not-encodable. Where the kernel or permissions refuse, entries report `permission_blocked`, and the suite stays green at the standard unprivileged paranoia level. Syscall-mode sampling reads one hardware group per PMU per sampling action; the enabled/running time pair is recorded alongside events so every metric discloses its multiplex ratio. Metric definition rows from the vendored tables ship as catalog data for a future release to ingest.

**Why this priority**: The richest provider and the reason for the vendored event data. It is last among measurement stories because the abstraction must land first, its acceptance needs Linux PMU access or its documented absence, and stories 1-5 carry the interface proof for every platform.

**Independent Test**: On a privileged Linux host, enumerate the catalog, assert named Intel or AMD events carry descriptions, open a group (instructions + cycles), measure, and check IPC against the ratio of the raw columns. In unprivileged CI (paranoia level 2), assert the provider reports `permission_blocked` states and the suite passes on fake and clock providers alone.

**Acceptance Scenarios**:

1. **Given** a Linux host, **When** the system opens, **Then** the catalog merges bundled table entries and kernel-exposed aliases, kernel-discovered entries win conflicts, and every entry carries a description.
2. **Given** a CPU identified by vendor, family, and model, **When** table selection runs, **Then** the mapping table selects the matching architecture directory, parsed once, lazily.
3. **Given** an event whose encoding fields exist in neither the vendored table nor the kernel's format descriptions, **When** resolution runs, **Then** the entry reports `not_encodable`; no partial encoding is attempted.
4. **Given** `perf_event_paranoid` at 2 and an unprivileged process, **When** availability probing runs, **Then** hardware entries report `permission_blocked`, clocks and push counters remain `countable`, and the test suite passes.
5. **Given** a group of resolved PMU events on one target, **When** a scope samples in syscall mode, **Then** one group read per PMU leader delivers all member values plus enabled/running times in the same action.
6. **Given** a multiplexed group (more events than physical counters), **When** a metric folds, **Then** the running ratio is below 1, the `scaled` flag is set, and the value is the scaled estimate the kernel computed.
7. **Given** plan construction over a leaf the catalog reports as not `countable`, or over a window a provider refuses to open, **When** group layout runs, **Then** construction fails with a recoverable error before any hardware read, the not-`countable` message naming the leaf address and the catalog state, the open message naming no member; **And** a plan binds one target, so a target or clock-id mismatch across group members has no spelling.

---

### User Story 7 - Near-single-instruction reads where the platform permits (Priority: P3)

The critical-path query must be near a single instruction wherever the platform allows. Each leaf gets a read mode at plan compile, from probing: `fast_tsc` (the rdtsc-grade clock, tens of cycles), `fast_rdpmc` (a pinned PMU event exposed through a read-only mapped page, read with the userspace counter instruction, tens of cycles), `syscall` (the fallback: group reads and vDSO clock reads, the microsecond regime), and `push_load` (in-instruction by construction). The catalog discloses the achieved mode per entry; plans compile against achieved modes; the same expression in fast versus syscall mode differs only in cadence budget. A fast value for an off-CPU multiplexed event is stale, and the enabled/running leaves make the fold disclose it in every mode.

**Why this priority**: This is the performance promise of the feature: `count(); code(); count();` means what it visually implies. It is P3 because it is a per-leaf mode layered on story 6's mechanisms, probe-gated, and skipped with the failure named where the kernel refuses.

**Independent Test**: On a fast-capable, probe-passing host, benchmark a `fast_tsc` + `fast_rdpmc` plan's `sample()` and show it lands in the tens-of-cycles regime while the same plan forced to `syscall` mode lands in the microsecond regime; both budgets recorded side by side. On any other host the benchmark reports its skip reason.

**Acceptance Scenarios**:

1. **Given** a leaf on a host where the probe passes, **When** the catalog reports it, **Then** its achieved mode is `fast_tsc` or `fast_rdpmc`; **And** where the probe fails, **Then** the mode is `syscall` and the catalog says so.
2. **Given** a fast-mode PMU leaf, **When** `sample()` runs, **Then** the read follows the mapped-page protocol: sequence-count retry, capability gate, index validity with a stated fallback path when the index reports not-allowed, offset adjustment, and counter-width masking.
3. **Given** a fast-mode plan, **When** `sample()` runs, **Then** the operation is a short in-instruction read sequence appending one column, and a benchmark shows fold cost entirely absent from the sample path.
4. **Given** a pinned fast leaf while its thread migrates or the event is multiplexed off-CPU, **When** a metric folds, **Then** the ratio disclosure reflects the enabled/running deltas exactly as in syscall mode.
5. **Given** fast-mode reads from a different thread than the one that opened the underlying context, **When** the plan or recorder is used, **Then** the per-thread binding contract rejects or documents the misuse as a construction-side or contract-level failure.
6. **Given** a fast-capable host in CI, **When** the fast-mode benchmark runs, **Then** tens-of-cycles and microsecond regimes are both measured and reported; **And** on a host where probing fails, **Then** the benchmark is skipped with the failure named.

---

### User Story 8 - Keep the vendored event tables current (Priority: P3)

The PMU event tables are vendored data under a version gate, byte-exact from the upstream source at a pinned revision. A first-class upgrade utility fetches a new upstream revision by reference, strips it to the in-scope architecture tables with a recorded exclusion list, validates every file parses, replaces the tree, rewrites the provenance record (reference, date, fetch URL, per-file hashes, exclusions), bumps the gate, and prints a per-architecture change digest so a reviewer sees churn at a glance. A `--check` mode verifies the tree against the record and the gate, exits nonzero on drift, runs in CI, and never fetches.

**Why this priority**: The tables age per microarchitecture, so currency is real maintenance. It ranks last because it serves the P3 PMU provider and changes no core behavior.

**Independent Test**: Run `--check` against the vendored tree (passes), corrupt a file or bump a gate constant without the tree (fails with drift named), run both modes against tiny synthetic fixture trees wired into CTest.

**Acceptance Scenarios**:

1. **Given** the vendored tree untouched, **When** `--check` runs, **Then** it exits 0; **And** when any vendored file's hash diverges from the record, **Then** it exits 1 naming the file.
2. **Given** the gate constant and the record disagreeing, **When** `--check` runs, **Then** it exits 1.
3. **Given** a valid upstream reference, **When** `--to <ref>` runs (developer-triggered, network available), **Then** the tree is replaced with byte-exact upstream files limited to the in-scope x86 tables, the record is rewritten with reference, URL, per-file hashes, and the explicit exclusion list, the gate constant is bumped, and the digest prints old-to-new event counts per architecture.
4. **Given** an upstream file that fails parse or schema sanity, **When** `--to` runs, **Then** the vendored tree is left untouched and the failure is reported.
5. **Given** CI, **When** any job runs the check target, **Then** no network fetch occurs; the check reads only the vendored tree and the record.
6. **Given** the synthetic fixture tree, **When** the tool's strip/check fixtures run under CTest, **Then** they pass and they fail on an intentionally drifted fixture.

---

### Edge Cases

- **Hardware wrap at 2^64**: a PMU counter wrapping between two points is handled by modular subtraction; the delta is correct and no violation fires. A wrap is expected physics; the contract covers it arithmetically.
- **Push-counter decrement between points**: a cumulative source going backwards is a developer error, checked at fold time as a contract violation.
- **Ring wrapped state**: folds over a ring recorder whose buffer wrapped must consult the dropped count; a fold that ignores drops reports a metric over a smaller window than the caller believes, so the check lives in the fold layer.
- **Invalid fold range**: `i >= j`, or `j` beyond the recorded extent, is a contract violation.
- **Scope misuse sequences**: `metric` requested on a window that is not closed, `finish` without `start`, and a second `start`: each is a contract violation, refused by the `SG_REQUIRE` at `source/counters/fold.cpp:289-290`, `source/counters/plan.cpp:344-345`, and `source/counters/plan.cpp:330`. A finished scope is a settled window, so US1 scenario 5's ten further `metric` calls fold the same two points. Registering a composite into a started scope has no spelling: `scope` exposes `start`, `finish`, `view`, and `metric` (`include/speedgun-ng/counters_measurement.hpp:981-1053`) and a composite reaches a window through the plan compiled before that window opens.
- **Name resolution failures**: unknown counter name, unknown object path, unknown object kind: recoverable errors with did-you-mean diagnostics built from catalog names and descriptions.
- **Duplicate registration**: duplicate counter name within one object, duplicate object path under one parent: recoverable errors.
- **Unrecognized catalog unit**: the unit-to-dimension mapping is a closed switch; an unrecognized unit is a resolution error, never a guessed dimension.
- **Provider yields a point smaller than the previous point for a hardware leaf**: treated as the wrap case under modular arithmetic; a non-modular backwards jump inside one provider read action is a provider-internal contract violation.
- **Paranoia-blocked hardware access**: catalog entries report `permission_blocked`; expressions over them fail at construction with the catalog state in the message; plans over `countable` leaves are unaffected.
- **Event present in table but unencodable by the running kernel** (missing format field): availability `not_encodable`; description still available.
- **Scaled-TSC platform**: the fast clock leaf reports its calibration provenance including the scaling flag.
- **Fast-mode off-CPU staleness**: disclosed through the enabled/running ratio leaves in every mode; the fold never presents a stale fast value as a fresh one.
- **Cross-thread recorder, plan, or push-counter use**: plans and recorders are per-thread objects, push counters are confined to the thread that samples them; PMU targeting binds at plan open; misuse is a contract violation.
- **Non-Linux platforms**: the same interface with a reduced catalog (clocks, push counters, fakes); zero API differences, and the empty PMU section is a catalog fact the user can branch on.
- **Recorder capacity of two points**: the minimum foldable recorder; `sample()` twice yields a one-interval fold. Capacity one is valid and holds a single point; a fold over it is impossible (`i < j` can never hold) and is a contract violation.
- **Zero-leaf expression or empty plan**: construction-time recoverable error.
- **Capacity exhaustion mid-loop under `hard_stop`**: aborts in every build configuration, release included; memory safety is never semantic-gated.

## Requirements *(mandatory)*

### Functional Requirements

**The system and its objects**

- **FR-001**: The system MUST present countable objects as a tree; each object MUST carry kind, canonical structured path, description, parent link, optional platform alias, and a catalog of named counters.
- **FR-002**: When the user resolves an object by canonical path or by platform alias, the system shall return the same object; canonical spelling MUST be used in all API results, provenance lines, and diagnostics.
- **FR-003**: When the user selects objects by kind with attribute filters, the system shall return exactly the matching objects. Filters are equality predicates that combine with AND; defined keys are the ancestor selectors `package` and `core` (matched against the canonical path components) plus attribute keys a provider declares for the selected kind; an unknown filter key is a recoverable error.
- **FR-004**: Every counter MUST belong to exactly one object; clock leaves MUST attach to the machine (root) object.
- **FR-005**: The catalog MUST report per entry: name, description, unit with its dimension mapping, availability state, and achieved read mode where applicable.
- **FR-006**: When a catalog entry is enumerated, the system shall report described-ness (from data) and countability (from probe, permission-aware) as separate predicates, with availability states at least: `countable`, `permission_blocked`, `not_encodable`, `absent`.
- **FR-007**: User code MUST be able to branch on catalog state alone; the public core API MUST require no compile-time platform branching.
- **FR-008**: When name or path resolution fails, the system shall return a recoverable error whose diagnostic suggests near-miss catalog names and descriptions.
- **FR-009**: The system MUST accept provider registration before open; catalogs MUST be immutable after open so concurrent catalog reads are safe by construction.
- **FR-010**: The public core vocabulary (catalog, algebra, dimensions, points, folds, provenance, recorder, plan) MUST NOT contain hardware-counter, clock-syscall, or platform-concept names; those MUST live only inside provider implementations.

**Provider contract**

- **FR-011**: When a provider's window reader samples, it shall yield for every managed leaf a cumulative raw `uint64` point plus the leaf's unit and metadata (description, availability, caveats such as multiplex times).
- **FR-012**: A new count source MUST be addable by supplying catalog entries (name, description, unit mapping) plus the window implementation, with no change to the core; the giraffe example MUST prove this from outside the library.

**Points, deltas, and the algebra**

- **FR-013**: Every leaf reading MUST be a cumulative point (all leaf kinds alike); when a delta is computed between two points, the library shall compute it as unsigned modular difference at 2^64, so a single hardware wrap yields the correct delta.
- **FR-014**: Only deltas MUST carry dimensions and enter the algebra.
- **FR-015**: The algebra MUST support addition, subtraction, division, and scalar scaling over resolved counters with compile-time dimension tags of the form `time^t x events^c`: addition and subtraction require identical tags, division subtracts exponents, scalar multiplication is unrestricted; `bytes + monotonic` MUST fail to compile.
- **FR-016**: Dimension tags MUST be construction-time only and erased on the read path (zero read-path cost).
- **FR-017**: When catalog unit metadata maps to a dimension, the library shall map it through a closed switch; an unrecognized unit MUST produce a resolution error and MUST NOT be guessed.
- **FR-018**: A metric MUST be a fold over deltas drawn from a recorded point sequence; the fold API MUST include the expression-member window fold `expr.fold(rec, i, j)`, a per-interval series fold `expr.fold_pairs(rec)`, and the first-to-last fold `expr.fold(rec)`; valid ranges require `i < j` within the recorded extent (tier-3 checked).
- **FR-019**: Every fold MUST return a result carrying value, running ratio, and scaled flag; for a source with an enabled/running pair the ratio MUST be computed from that delta pair; a source without such a pair (clock, push, fake) MUST disclose ratio 1.0 with scaled false; a composite's ratio MUST be the product of its constituent ratios each raised to its algebraic exponent; the disclosure fields MUST be structurally impossible to omit.
- **FR-020**: Every composite MUST expose each constituent leaf's raw point column with provenance: object path, name, description, unit, raw values, point identity, and multiplex ratio.
- **FR-021**: Expression construction MUST perform zero hardware reads; folds MUST compute only on demand, MUST be callable any number of times and for any subset after measurement, and MUST NOT trigger provider reads.

**Plans and recorders**

- **FR-022**: When a set of expressions is compiled, the system shall produce a flat read plan (leaf slots, grouping layout, fold sequence, compact column layout); the read path MUST contain no expression tree and no name lookup. A read group is entered through the direct-call thunk its window installed in the window constructor, so the five shipped windows (clock, push, fake, PMU group, PMU mapped page) reach `read_points` with no vtable lookup. The seam keeps a documented fallback for a provider window that installs no thunk (`include/speedgun-ng/counters_provider.hpp:245-251`, `:326-337`); such a window reaches `read_points` through the vtable at a cost of one lookup per sampling action, which is the price of implementing the window contract alone, and the giraffe example runs on that path (`example/counters_giraffe_example.cpp:46-57`).
- **FR-023**: The system MUST assign each leaf a read mode at plan compile, chosen by probe: `fast_tsc`, `fast_rdpmc`, `syscall`, or `push_load`; achieved modes MUST be recorded and disclosed in the catalog.
- **FR-024**: One plan MUST bind exactly one sampling target, and every read group and every read mode in that plan MUST open against that one target, so a target or clock-identity mismatch across group members is unrepresentable; a leaf the catalog reports as not `countable` and a window a provider refuses to open MUST be recoverable construction errors, never a read-time surprise. The not-`countable` error names the leaf address and the catalog state that refuses it. The open error is one diagnostic per read group, reporting that the provider could not open a window for the leaves it owns and naming no leaf, no provider, and no kernel reason, so a caller that needs the offending member narrows the refusal by compiling its leaves one at a time.
- **FR-025**: A recorder MUST be created from a plan through one factory taking a compile-time overflow-policy tag (default `hard_stop`, opt-in `ring`) and a capacity measured in point columns (capacity 1 holds a single sample; folds require capacity at least 2); call sites MUST NOT require visible template arguments.
- **FR-026**: `recorder.sample()` MUST be the named critical-path operation: `noexcept`, zero allocation, zero lock; a fast-mode sample MUST be a short in-instruction read sequence appending one column; a syscall-mode sample MUST perform one group read per PMU leader plus vDSO clock reads plus plain push loads within one sampling action.
- **FR-027**: Under `hard_stop`, sampling past capacity MUST abort through an always-enforced contract check (present in every build configuration, release included).
- **FR-028**: Under `ring`, capacity MUST be a power of two (enforced at construction), the write index MUST mask branchlessly, wrapped state and dropped count MUST be recorded, and folds MUST account for drops.
- **FR-029**: A recorder MUST be a small value handle over a plan-arena buffer allocated at construction; multiple recorders of one plan MUST get independent buffers sharing the compiled layout.
- **FR-030**: The scope spelling (`start`/`finish`/`metric`) MUST behave exactly as a two-point recorder: one semantics, two spellings.
- **FR-031**: Plans MUST bind hardware targeting (thread or cpu) at plan open; plans and recorders MUST be per-thread objects; multiple plans over one system MUST be first-class.
- **FR-032**: When a plan's measurement window is calibrated, the library shall expose the per-plan read-path overhead so every reported number carries its own measurement floor; fold windows include their endpoints' sample cost, stated in documentation.

**Shipped providers**

- **FR-033**: The library MUST ship a `clock` provider offering monotonic wall time, thread CPU time, and process CPU time on the machine object, dimension `time^1`, requiring no privileges.
- **FR-034**: The `clock` provider MUST offer a fast `tsc` leaf (rdtsc-grade) whose frequency is calibrated at provider construction from platform data; registration is refused after open and the catalog freezes there (FR-009), so the constructor is the last boundary at which a leaf can be seeded, and a calibration deferred to open would publish an uncalibrated leaf. Calibration provenance and any scaling flag are reported in the catalog.
- **FR-035**: The library MUST ship a `push` provider whose counter is confined to a single thread: `add(n)` is a plain non-atomic increment and the sample-time read is a plain non-atomic load, with no atomic read-modify-write on any path; `add()` or sampling from a thread other than the owning thread MUST be a tier-3 violation; a decrement of a push counter between points MUST be a tier-3 violation.
- **FR-036**: The library MUST ship a deterministic `fake` provider, hand-driven, sufficient to test all catalog, algebra, dimension, recorder, fold, and provenance logic without sleeps or privileges.
- **FR-037**: The library MUST ship a `linux_pmu` provider whose catalog merges bundled vendored tables with kernel-discovered aliases (kernel discoveries winning conflicts), encodes events by composing table semantics with the running kernel's format bit layouts, and marks events lacking required format fields `not_encodable`.
- **FR-038**: When the `linux_pmu` provider opens, the library shall select the architecture event table by CPU identification through the upstream mapping file, parsing only the matched table, once, lazily.
- **FR-039**: The `linux_pmu` provider MUST compute availability per entry by test-opening events and reading permission state, reporting `permission_blocked` where refused.
- **FR-040**: The `linux_pmu` fast mode MUST follow the mapped-page userspace-read protocol: sequence-count retry, capability gating, one-based index validity with a stated fallback when not allowed, kernel offset adjustment, counter-width masking, and per-thread same-thread context binding.
- **FR-041**: Enabled-time and running-time MUST be modeled as ordinary cumulative leaves sampled with their groups; every multiplex ratio MUST be computed as their delta quotient inside folds, in every read mode.
- **FR-042**: On non-Linux platforms the provider set MUST reduce to clocks, push, and fake behind the identical interface; the reduced catalog MUST be the whole difference.

**Vendored tables and the upgrade tool**

- **FR-043**: The event tables MUST be vendored byte-exact at a pinned upstream revision under `external/pmu-events`, guarded by a version gate that fails the build until the recorded constant matches the tree (the established gate pattern).
- **FR-044**: The upgrade utility MUST provide a fetch mode taking an upstream reference: fetch the event-table path only, strip to in-scope x86 tables plus the mapping file with an explicit recorded exclusion list, validate parse and schema sanity before replacing anything, replace byte-exact, rewrite the provenance record (reference, date, URL, per-file hashes, exclusions), bump the gate constant, and print a per-architecture old-to-new change digest.
- **FR-045**: The upgrade utility MUST provide a `--check` mode verifying tree hashes against the record and the gate constant against the reference, exiting nonzero on drift, wired into CI, performing no network access; synthetic fixtures MUST cover both directions through CTest.

**Contracts, threading, cadence, standalone**

- **FR-046**: Enforcement MUST follow three tiers: dimension violations and the fold-result shape are compile-time; catalog resolution failures and registration duplicates are recoverable errors; scope misuse, capacity overrun under `hard_stop` (FR-027), push decrements, invalid fold ranges, and cross-thread misuse are contract violations that terminate in dev/CI.
- **FR-047**: Each recorded sample column MUST satisfy the invariant that all leaf values in the column were read within one sampling action under one plan binding; folds MUST consume columns of a single recorder.
- **FR-048**: Sampling cadence MUST be caller-owned; the documented idiom for tight loops MUST be chunked sampling with capacity `N/K + 1` and per-interval series folds, with the K=1 observer-effect cost stated numerically from the plan's overhead calibration.
- **FR-049**: The public surface MUST be standalone: an example using only the public headers and the standard library MUST compile, run, and fold a metric whose dynamic dependencies are this platform's C and C++ runtime alone; the target is a static archive, so the link manifest (`ldd`, `readelf -d`) carries no `speedgun-ng` entry and no third-party entry; no benchmarking-framework code, API, or dependency appears anywhere in 007.
- **FR-050**: The class design MUST remain directly embeddable in fixed-iteration, per-thread benchmark loops: all construction in the untimed setup region, recorder capacity computable from a known iteration count, per-thread plans, and fold results usable as per-iteration counter inputs.

### Key Entities

- **System**: the root handle to the local machine's counting world; holds the object tree, the merged catalog, provider registry; immutable after open.
- **Countable object**: a named, described, typed node (`machine`, `package`, `core`, `uncore_imc`, `amd_iommu`, `menagerie`) owning a counter catalog; canonical structured path plus optional platform alias; every counter belongs to exactly one.
- **Catalog entry**: one named counter on one object: name, description, unit with dimension, availability (described vs countable-now), achieved read mode.
- **Provider**: an implementation of the window contract registering objects and counters and yielding cumulative points; built-ins: clock, push, fake, linux_pmu; user sources are first-class.
- **Counter / expression**: a resolved leaf, or a typed composition (sum, difference, quotient, scalar scale) of resolved leaves; dimension-checked at construction; erased at read.
- **Point**: one leaf's cumulative `uint64` reading at an instant; the only thing hardware, kernels, and users ever yield.
- **Delta**: the modular difference of two points; the only thing carrying dimensions or entering folds.
- **Plan**: the compiled read program: leaf slots, group layout, read modes, fold sequence, arena layout; per-thread binding; produced once, in the untimed region.
- **Recorder**: a value handle over the plan's column buffer; `sample()` appends one column of points; overflow policy `hard_stop` or `ring` fixed at construction.
- **Metric result**: the fold output: value, running ratio, scaled flag; structurally complete.
- **Provenance record**: per-leaf raw view: object path, name, description, unit, raw values, ratio; attached to every composite.
- **Event table record**: the vendored data tree's provenance: upstream reference, fetch URL, per-file hashes, exclusion list; verified by the check mode.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A developer with only this library and the standard library writes, builds, and runs a program that measures a composed metric (an `instructions / cycles` shape over fake and clock sources) in a manual loop; the standalone example passes in CI, and its link manifest names the platform C and C++ runtime only, with no `speedgun-ng` entry and no third-party entry.
- **SC-002**: The complete test suite passes on an unprivileged CI runner at the standard hardware-access paranoia level: fake, clock, and push providers carry every assertion; PMU-dependent assertions verify `permission_blocked` states and pass.
- **SC-003**: The giraffe provider example registers objects and counters out of tree and measures `honks / monotonic` with zero modifications to library core sources.
- **SC-004**: Both budgets are measured and published. On the reference Linux host, a clock-only plan reading one leaf per sampling action publishes a stated nanosecond distribution (min/median/max) over 257 timed actions; the same page publishes a core-PMU group plan's distribution and a fold-only figure taken off the sampling path. Where the host's probe passes a fast read mechanism, the benchmark measures a second one-leaf plan through that mechanism, publishes both distributions side by side with the ratio between the two medians, and the pass check is binary on that host: the fast-mode median is below the syscall-mode median. The two plans read different leaves, and no fixed factor separates their medians: a `sample()` action carries the recorder bookkeeping and the contract checks on top of the read, and the syscall medians this tree records are tens to hundreds of nanoseconds. Where the probe refuses the fast mechanism, the fast row stays unmeasured, the benchmark exits 2, and the page names the gating probe reason.
- **SC-005**: `sample()` performs zero allocations, proven by an allocation-counting test in the suite.
- **SC-006**: A hand-built point buffer (fake provider) folds to exactly the expected metrics for known sequences, including a crafted 2^64 wrap and a per-interval series: zero tolerance on these fixtures.
- **SC-007**: One fan-out window yields IPC for every core on a multi-core host; per-core instruction deltas reconcile against the shared total within the plan's calibration.
- **SC-008**: Every metric number reaching a caller carries ratio and scaled disclosure, and every composite's raw columns are inspectable with provenance: a test extracts `IPC = 1.31 <- instructions 12.3e9 / cycles 9.4e9, ratio 0.98`-style records end to end.
- **SC-009**: The table check tool runs in CI on every change, catches a synthetic drift fixture with exit 1, and completes without network access.
- **SC-010**: A benchmark demonstrates the hot-loop idiom `sample(); code(); sample();` with fold cost entirely absent from the sample path's measured cost.

## Assumptions

- The closed journal `sg_counters.md` (2026-09-25) is the authoritative design record; where this spec compresses it, the journal's resolved scope statement and decision entries govern intent.
- Deferred to future specs, and excluded from 007 acceptance: latency histograms, string-formula metric ingestion (the future grammar is the perf MetricExpr subset; a second dialect will not exist), the benchmark harness and registration API, report serialization, hwloc-fed object trees, multiplexing-aware group scheduling heuristics, Windows/macOS hardware-PMU providers.
- Full TSC-based wall-clock conversion machinery beyond what the fast clock leaf needs remains out of scope; the leaf itself is in scope.
- The vendored event tables come from the kernel's `tools/perf/pmu-events` tree (dual MIT/GPLv2 data); a licensing confirmation pass during planning is assumed to conclude BSD-3-compatibility; if it does not, the vendoring step is blocked and the PMU provider falls back to sysfs-discovered entries with the same interface.
- Hardware counter access on Linux requires `perf_event_paranoid` relaxation or `CAP_PERFMON` for the privileged acceptance tests; CI runs unprivileged, and the fake/clock providers are the documented substitute.
- Kernel support for the userspace counter-read page (capability, index, pinning) is verified by probe at runtime, per-host, by opening one event and reading the page the kernel maps for it. No sysctl and no sysfs attribute takes part in the read, and no assumption about the host's permission level is made: `perf_event_paranoid` gates the availability probe's test-opens, and the kernel's own `cap_user_rdpmc` bit gates the mapped-page read. A claim about which level grants a mapped-page read is a folklore version number and is treated as unreliable.
- The `tsc` clock leaf is published only on a host whose kernel exposes a calibrated time-stamp frequency, which means a kernel built with `CONFIG_X86_TSC=y` and `CONFIG_CALIBRATE_TSC=y`; that configuration publishes `/sys/devices/system/cpu/tsc_khz`. A host without it carries no `fast_tsc` leaf, which is a catalog fact with zero API difference (FR-034), and no acceptance criterion may assume the leaf is reachable.
- Integer counts beyond 2^53 lose exactness inside the double fold; at sustained one-billion-per-second counting that is roughly 104 days in one scope; scopes that long are treated as aggregates, and the limitation is documented.
- Reference hardware for performance budgets is the maintainer's x86-64 Linux development machines; budgets are published per-platform per constitution VII as baselines land.
- Google-Benchmark-style loop embeddability is a binding class-design constraint (FR-050) with zero 007 code written against any benchmarking framework; the integration glue belongs to the future harness spec.
