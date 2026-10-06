# Feature Specification: Counters Defect Resolution

**Feature Branch**: `012-counters-defect-resolution`

**Created**: 2026-10-03

**Status**: Draft

**Input**: User description, held at `speedgun-ng-012-specify-prompt.md`
, quoted verbatim:
<!-- prose-lint: allow reason="XI.5 verbatim quotation of the request, marked as quoted" -->
"Counters defect resolution, the precursor to the benchmark harness. Spec
007 deferred the benchmark harness and its registration API to a future
spec. It also bound the counters classes to stay embeddable in
fixed-iteration, per-thread benchmark loops (007 FR-050). That harness is
the next spec after this one. It will call `sample()` and `fold` across
millions of iterations and run for hours. It will compile per-thread and
per-cpu plans from worker threads, report the multiplex disclosure beside
every number, and link the installed package. An audit of the counters
library at `6aafd2d` found defects on each of those paths. This feature
makes the counters library a measurement substrate the harness can trust.
Every value it hands a caller is correct or is disclosed as unavailable.
Principle VI states that a wrong benchmark is worse than no benchmark."

The rest of that document holds the ten defects with their source sites,
the scope, the constraints, the success criteria, and seven open
questions. It stays the audit record this specification answers.

## Purpose

The counters library becomes a measurement substrate the benchmark
harness can trust. Every value it hands a caller is a correct value, or
the library discloses that the value is unavailable.

Ten defects carry stable identifiers, I-01 through I-10. Each one
corrects behaviour the shipped providers already claim, and each one
restores a requirement that specs 007, 008, or 011 already state.

- P1 defects block harness correctness and land first: I-04, I-05, I-06,
  and I-08.
- P2 defects widen the hosts and deployments the harness serves: I-01,
  I-02, I-03, I-07, I-09, and I-10.

A defect is **confirmed** where the cited lines show the defect. A
defect is **suspected** where the code supports the reading and no run
on the affected hardware has shown it. Clarify or plan confirms each
suspected defect. A suspected defect that does not reproduce leaves the
scope, and the plan records the evidence.

Every correction is recorded in the successor log at
`specs/007-counters-and-timers/citations-log.md`, against the 007, 008,
or 011 requirement it restores. Spec 008 corrected 007 the same way.

## Clarifications

### Session 2026-10-03

- Q: How does the vendored event table data travel with an installed
  package? → A: The build embeds the tables into the archive. The
  installed library is an archive whose code links into the caller's
  executable, so the archive has no location at run time and no reliable
  path to resolve from. Embedding removes the path lookup, and the
  installed package then publishes the same rows as the build tree,
  which SC-008 requires. A configured data path adds public surface and
  a knob the harness gains nothing from.
- Q: How does availability express the target kinds an entry supports,
  and what is the fallback for a refused fast read? → A: Availability
  names the target kinds the entry supports and separates a scope
  refusal from an encoding refusal. A refused fast read discloses a gap
  through one managed disclosure column, written through the same `put`
  the ratio pair already uses. A gap keeps the sampling action at its
  published cost and adds no per-sample metadata. A syscall fallback
  would make the fast-read cost unpredictable, and Principle VII tracks
  that figure. The harness can report a gap; a hidden cost change is
  invisible to it.
- Q: How should the build embed the vendored event tables into the
  library archive? → A: The archive carries the raw JSON bytes as static
  data, and the existing simdjson parse path decodes them at run time.
  The vendored tree holds 542 JSON files totaling 24.5 MB across 40
  architecture directories. A generated C++ row array ships the same
  rows at a larger archive size and a slower compile, and embedding one
  architecture directory alone drops rows SC-008 needs. The parse code
  in `source/counters/linux_pmu/table_parse.cpp` already reads these
  files with simdjson, so this form adds no codec and no dependency.
  Every executable that links the library carries the added size, and
  FR-023 makes the plan record it under Principle VII.
- Q: Should the build carry a CMake option that turns the embedded
  tables off? → A: The embedding is unconditional. An option would make
  FR-023 and SC-008 hold on the default build only, and it would add a
  second configuration that needs its own gates. X.2 bars
  configurability no caller needs, and no caller needs a smaller binary
  today. A later specification adds the option when a caller needs it.
- Q: How does SC-004 count real file descriptors and mappings when
  FR-034 requires every test to run unprivileged at
  `perf_event_paranoid` 2? → A: Both tests ship. A synthetic test runs
  on every job and holds the coverage gates. It counts
  `/proc/self/fd` and `/proc/self/maps`, and the resources it opens for
  the plan stand in for event descriptors, so it measures real release
  on a host where the kernel refuses `perf_event_open`. A host-dependent
  test measures the real event descriptors and mappings on a host that
  grants the event. Option A alone would prove a counter decremented.
  Option B alone skips on every CI runner that refuses the event, and
  the release path then carries no branch coverage.
- Q: What states does the catalog publish in an entry's availability,
  and does publishing them change the public surface of an entry? → A:
  One enumeration holds the countability state, with a separate value
  for a scope refusal and a separate value for an encoding refusal. The
  targets an entry can be counted on travel beside it as a fixed-size
  bitmask over the target kinds, so a new kernel target adds a bit
  and no enumeration value changes. Reading the mask costs no
  allocation. One enumeration holding both concerns grows a value per
  target, and two enumerations make every caller hold two fields in
  step. This adds public surface, so the version bump FR-024 names
  applies.
- Q: Does the thread-sanitizer run that SC-003 requires land as a new
  CI preset and job, or does it stay a developer-local run? → A: A new
  preset configures the thread sanitizer and a new CI job builds and
  runs the suite under it, so FR-012 and SC-003 become gates CI
  enforces on every change. The thread sanitizer cannot share a build
  with the address sanitizer, because the compilers reject that
  pairing, and the existing sanitizer preset keeps address and
  undefined behavior alone. The preset SHALL either build the vendored
  trees with the thread-sanitizer flags or carry a suppressions file
  under version control, because the sanitizer reports races in
  uninstrumented vendored code such as simdjson and quill, and a job
  without one of the two produces false findings (Principle VIII).
- Q: Does FR-007 widen `point_sink::put` to carry the disclosure, or
  does the disclosure travel without changing the sink's interface? → A:
  The disclosure travels without changing the interface, as one managed
  column written through the existing `put`, and `put` keeps its
  one-integer signature. A sentinel count would publish a wrong value
  with no disclosure, against Principle VI, and a second channel would
  make the caller hold two sources in step.
- Q: Does this feature give the harness a recorder it can reuse across
  repetitions, or does the harness specification own that? → A: The
  harness specification owns recorder reuse. It holds the repetition
  model, so it is the right place to design reuse. The decision keeps
  this feature a defect-fix feature with no new API, and FR-009 records
  it as out of scope.

### Session 2026-10-04

- Q: Must the sampling thread of a cpu-target fast-mode plan stay on
  the processor the plan opened its contexts on? → A: Yes. The
  requirement is a semantic-gated precondition on the caller. It adds
  no run-time check to the hot path. Dev and CI builds catch a migrated
  thread through `SG_REQUIRE`, and a release build configured `ignore`
  emits no code, so the per-sample cost stays where FR-008 holds it.
- Q: Does the timestamp-counter clock leaf gain a read fence? → A: No.
  Its contract states the precondition that one thread takes both
  window endpoints, and the per-read cost stays where it is. A fence
  helps only a caller that reads across threads, and the harness does
  not read across threads.
- Q: What measured difference from the published per-sample cost figure
  counts as a regression that FR-008 fails on? → A: A median more than
  5 percent from the recorded figure, measured as the median over
  repeated runs at a fixed iteration count on a pinned processor. A
  single run on a shared host moves further than that. The plan records
  the run method beside the figure.
- Q: Should the encodable-row counts for the four named Intel
  architecture directories be pinned in the spec or recorded as a
  measured baseline? → A: Measured. The plan measures each count over
  the pinned tree after the correction, and the fixture pins those
  exact numbers. Asserting only that each count is above zero would
  pass against a catalog that encodes one row.
- Q: Where does the gap disclosure reach the caller, given that the
  published point carries no ratio field and no availability field? → A:
  One managed disclosure column per sampling action, written through the
  existing `point_sink::put`, the mechanism the `enabled` and `running`
  ratio pair already uses. `put` keeps its one-integer signature, so
  007 FR-011 holds and the disclosure adds no per-sample metadata. The
  premise behind the earlier answer on this question was wrong. `put`
  takes one `std::uint64_t`
  (`include/speedgun-ng/counters_provider.hpp:201`), the ratio rides two
  ordinary columns (`source/counters/linux_pmu/group_io.cpp:258-262`),
  and availability is a field fixed at registration
  (`include/speedgun-ng/counters_provider.hpp:44`), so there was nothing
  to widen `put` with. A sentinel count would publish a wrong value with
  no disclosure, against Principle VI. A second channel would make the
  caller hold two sources in step.
- Q: Which kernel format set do the pinned Intel encodable-row counts
  get measured against? → A: A named synthetic sysfs format list the
  fixture supplies, so one number gates every host on the matrix. FR-034
  already asks a test that touches the kernel for synthetic sysfs. A
  count taken against a runner's own formats differs from the reference
  host's, so no single figure would gate both.
- Q: Which figure and which plan does FR-008 measure against? → A: The
  plan measures each gated plan at the pre-fix head `6aafd2d` on the
  reference host and again after the correction, with one run method
  across both measurements, and records both figures. Two plans are
  gated: the core PMU group over `cpu/instructions` and `cpu/cpu-cycles`
  with one read per leader per action, because the harness reads that
  path, and the clock-leaf plan over `machine/monotonic` beside it. A
  figure another session recorded under a load this feature never ran is
  not a baseline, and a baseline written after the change cannot
  regress.
- Q: Which Intel host confirms I-01 and I-04 on hardware? → A: None yet.
  The plan records the confirmation as deferred, names the host class
  that would settle it, and records that the repository owner expects to
  add an Intel host later. No requirement, test, or gate in this feature
  depends on the confirmation, so the deferral closes the decision with
  the suspected parts of both defects left in scope.
- Q: Which version does this feature ship as, now that the disclosure
  travels in a managed column and the only public change left is the
  availability surface? → A: 0.3.0, the next minor bump, with
  `SOVERSION` staying at 0. Spec 008 shipped a minor for a change that
  added a member to `system` and changed no signature, which is the
  same shape as adding the target-kind bitmask beside the availability.
  Spec 011 called its purely additive change patch-level, and the plan
  records why that wording does not extend to a field added to a public
  record.

### Session 2026-10-04 (convergence)

- Q: How does the disclosure column express a per-action gap when the
  countability state names a per-entry condition? → A: One enumerator
  joins the availability state and names the per-action case: `gap`. A
  measured action writes the entry's own countability value, which is
  what the existing answer required. An action that measured nothing
  writes `gap`. The catalog never publishes `gap` for an entry, because
  a gap is a property of one action and an entry spans many. The
  earlier answer that one enumeration carries both concerns stays, and
  one value of it names the action case.
- Q: How does a caller tell a real zero count from a gap? → A: The zero
  count carries `gap` beside it, and the measured zero carries the
  entry's own countability value beside it. A sentinel count publishes
  a wrong value with no disclosure, which Principle VI forbids, so the
  sentinel stays rejected.
- Q: Which code proves that the corrected decisions on a
  kernel-granted path reach the coverage gates? → A: A small pure
  function declared in the project's existing private seam header,
  called by the kernel-facing wrapper and driven directly by a
  registered test over synthetic input. The repository already holds
  two such functions, one that decides whether a fast read is
  permitted and one that decides whether a seqlock read is stable, and
  a registered test covers both arms of each. FR-046 names the shape
  and the pairing.
- Q: Does the correction remove the coverage-exclusion markers that
  stand on the corrected paths? → A: The markers covering the
  release arms go, because a registered test now reaches them. The
  markers covering a wrapper that only a granted event can enter stay,
  because removing one of those makes the coverage gate fail on the
  runner that refuses the event and on the developer host that grants
  it. The two together cover what neither host covers alone. FR-027
  counts markers, and this feature adds none.
- Q: How does the fast-window destructor reach the coverage gate on a
  runner that refuses `perf_event_open`? → A: The release path
  forwards a mapping address, a length, and a descriptor to `munmap`
  and `close` and reads nothing else. A registered test opens its own
  descriptor and its own anonymous mapping, places them in a context
  value, and lets the value leave scope. The destructor performs a
  real release on every host, with no privileged event and no new
  production symbol.
- Q: Does the per-sample cost bound become a gate that runs on every
  change? → A: No. The bound is a measurement on the reference host,
  recorded beside the pre-fix figure under one run method. A
  registered test asserts the properties a CI runner can decide: that
  the sampling action is `noexcept`, allocates nothing, and takes no
  lock. Constitution VI requires every test to be deterministic, and a
  timing constant measured on one host cannot decide a test on
  another.
- Q: How does a correction prove it fails before it lands, given that
  the pre-fix tree holds no test for it? → A: The test is written at
  the current head with the correction unapplied, and the test run
  observes the failure. Checking out the pre-fix head discards the test
  the observation needs, so no task checks out the pre-fix head to
  observe red.
- Q: Which Intel architecture directories does the confirmation
  deferral name, and what settles it? → A: Unchanged. The deferral
  stands, the host class is named in the plan, and no requirement,
  test, or gate depends on it.
- Q: Which value does the disclosure column carry for an action that
  measured nothing? → A: The availability state gains exactly one
  value for the per-action gap case, named `gap`. A measured action
  writes the entry's own countability value. An action that measured
  nothing writes `gap`. The catalog never publishes `gap` for an
  entry. This rejects the queue's recommended Option A, which gave
  each of the four failure causes its own value. Principle X.2
  forbids carrying a distinction no caller needs, and the caller
  needs to learn that the value is unavailable and nothing further.
  FR-002 forbids a retry of the same event through a syscall, so the
  cause changes no caller behaviour, and Principle VI is satisfied by
  a correct value or a disclosed unavailability. One value keeps three
  obligations: the doxygen clause on the enumerator, the paired
  enforcement site, and the closed-switch arm that prints the state's
  name.
- Q: Which kernel format names go into the synthetic sysfs format list
  the Intel counts measure against? → A: A literal transcription of
  the format file names the reference host's kernel publishes under
  sysfs, held in the fixture as a named list, so the count depends on
  the fixture alone and one number gates every host on the matrix.
- Q: In one sampling action, in which order are the count columns and
  the disclosure column written? → A: The disclosure column is written
  last, after the counts and after the ratio pair's two columns. This
  rejects the queue's recommended Option A, which wrote the disclosure
  first. `point_sink::put` appends the next managed column and a fold
  addresses every column by index after the action commits, so
  publication order does not order a fold's reads. A sink that met the
  disclosure first still reads whichever column it chose first.
  FR-006's guarantee comes from the fold consulting the disclosure
  column and from the window publishing no count the read never
  produced, and neither depends on the write order. The order also
  holds the hot path's write sequence where FR-008 measures it, which
  Principle VII requires.
- Q: SC-001 bounds the ratio over a multiplexed window with a tolerance
  the specification names no unit for. Which unit does the bound carry?
  → A: One nanosecond, the unit the kernel's own time-enabled and
  time-running fields already carry, so the comparison needs no
  host-dependent conversion.
- Q: What baseline do the `amdzen4` and `amdzen5` encodable-row counts
  get held against? → A: The counts measured at the pre-fix head over
  the pinned tree, measured against the same named synthetic format
  list the fixture supplies, recorded in the plan and pinned in the
  fixture.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Receive a fast-path sample that is a count or a disclosed gap (Priority: P1)

A benchmark harness samples a fast-mode counter once per iteration and
folds the points into per-iteration costs. One sampling action among
millions is enough to corrupt every fold that spans it. A run of several
hours is long enough for a counter to wrap at its published width, and a
multiplexed event reports a ratio of one for a window it shared with
another event.

The harness author needs each point to be a real cumulative count, and
needs every refused, unstable, or failed read to reach a documented
fallback.

**Why this priority**: Every iteration of a fast-mode plan passes
through this read. The four parts of I-04 and the defect in I-05 sit on
the one path the harness cannot avoid.

**Independent Test**: Drive the fast read over synthetic event pages and
synthetic groups: a page whose capability is absent, a page whose read
is refused, a page whose sequence moves, a counter near its width
boundary, a multiplexed window, and a group whose read returns fewer
bytes than its header. Compare every result with the recipe the kernel's
own interface header documents. The fixture runs unprivileged.

**Acceptance Scenarios**:

1. **Given** a fast read the page refuses, **When** the sampling action
   runs, **Then** the point takes the documented fallback, and the value
   of an earlier read is discarded.
2. **Given** a page whose sequence moves on the first read, **When** the
   sampling action runs, **Then** the point takes the documented
   fallback.
3. **Given** a counter that has advanced past its published width,
   **When** two points are folded, **Then** the delta equals the true
   advance.
4. **Given** a window in which the event could run on part of the
   elapsed time, **When** the pair of times is read, **Then** the
   reported ratio matches the kernel's documented time computation.
5. **Given** a group read that returns fewer bytes than the group
   header, **When** the sampling action runs, **Then** no fold across
   the surrounding points reports a delta above the counts the fixture
   drove.

---

### User Story 2 - Resolve counters and compile plans from many threads at once (Priority: P1)

A benchmark harness resolves counters and compiles its own per-thread
and per-cpu plans on worker threads, after the catalog is open. Today a
catalog lookup and every plan compile write shared state, with no
synchronization, so two workers corrupt each other's resolution.

The harness author compiles plans concurrently and expects the library
to be safe by construction, because specs 007 and 011 already promise
that use.

**Why this priority**: The defect is a data race on the harness's normal
execution path. A race produces wrong numbers at random and survives
every functional test.

**Independent Test**: Open the catalog, then run several threads that
resolve counters and compile plans concurrently, under a thread
sanitizer. The run reports no race.

**Acceptance Scenarios**:

1. **Given** an open catalog, **When** several threads resolve the same
   and different canonical addresses at once, **Then** every resolution
   returns the entry the catalog holds, and no thread observes a
   half-built entry.
2. **Given** an open catalog, **When** several threads compile plans at
   once, **Then** every compile succeeds or reports its own error, and
   no compile writes shared state.
3. **Given** an open catalog, **When** one thread walks an object's
   parent and children while another thread resolves an address,
   **Then** both complete and return the published relationships.

---

### User Story 3 - Compile and destroy plans without exhausting host resources (Priority: P1)

A benchmark harness compiles a plan per benchmark, per thread, and per
repetition. A plan that opens its fast-mode windows holds one event
descriptor and one mapping per group member, and a plan that is
destroyed releases neither today.

The harness author compiles and destroys plans in a loop and expects the
process descriptor count and mapping count to stay at their starting
values.

**Why this priority**: The harness exhausts the descriptor limit long
before it finishes, and the failure looks like a resource shortage in
unrelated code.

**Independent Test**: Open and destroy a fast-mode plan 10,000 times,
then compare the process descriptor count and mapping count with their
values before the loop. A partial open, where a later member fails, runs
the same comparison.

**Acceptance Scenarios**:

1. **Given** a fast-mode plan that opened every member, **When** the
   plan is destroyed, **Then** every descriptor and every mapping the
   plan acquired is released.
2. **Given** a fast window whose later member fails to open, **When**
   the failed open returns, **Then** the descriptors and mappings the
   earlier members acquired are released, and no partial window is
   published.
3. **Given** a plan destroyed before it was opened, **When** the
    destruction runs, **Then** nothing is released and no check fails.
  4. **Given** a host whose kernel grants the fast event, **When** the
    lifecycle test runs against real event descriptors and mappings,
    **Then** the descriptor count and mapping count return to their
    starting values.
  5. **Given** a host whose kernel refuses the fast event, **When** the
    lifecycle test runs over substituted descriptors, **Then** the
    descriptor count and mapping count return to their starting values,
    and the test that needs the real event reports that it could not
    run.

---

### User Story 4 - Count events on hybrid, uncore, and Intel hosts (Priority: P2)

A benchmark harness runs on hybrid hosts, on hosts with uncore metrics,
and on Intel hosts. Today the vendored Intel event tables encode no
rows, the vendored rows reach only the device named `cpu`, and every
availability probe asks as though the calling thread were the only
target.

The harness author counts events on every device the host publishes, and
expects every row of the catalog to encode on an Intel host.

**Why this priority**: These defects decide which hosts the harness
serves. They leave AMD hosts correct, and the reference host is AMD, so
the suite never sees them.

**Independent Test**: Over the pinned table tree, publish the
encodable-row count for each Intel core architecture directory and
assert the counts for four named directories. Drive synthetic hybrid and
uncore device fixtures and assert each row lands on the device its scope
names. Drive a device-scoped entry and assert the catalog separates a
scope refusal from an encoding refusal.

**Acceptance Scenarios**:

1. **Given** a catalog entry whose event the fast instruction cannot
   read, **When** a caller reads the catalog, **Then** that entry
   reports the syscall read mode, and no other entry changes mode
   because of it.
2. **Given** an Intel event table row whose keys are sampling keys and
   metadata, **When** the catalog loads the table, **Then** the row
   carries no encoding obligation. The keys that carry none are
   `SampleAfterValue`, `MSRValue`, `MSRIndex`, `CounterMask`, `Invert`,
   `EdgeDetect`, `PEBS`, `Data_LA`, `PerPkg`, and `Experimental`.
3. **Given** an event table row whose keys name kernel formats, **When**
   the catalog encodes the row, **Then** each key reaches the kernel
   format it names.
4. **Given** a row that needs a field the running kernel does not
   publish, **When** the catalog encodes the row, **Then** the row
   reports `not_encodable`, and a row whose fields the kernel publishes
   never reports it.
5. **Given** a hybrid host publishing core and atom devices, **When**
   the catalog loads a core-scoped row, **Then** the row appears on each
   core device the scope applies to.
6. **Given** a row scoped to a device the host does not publish,
   **When** the catalog loads, **Then** the row stays out of the
   catalog.
7. **Given** an uncore-scoped row and a core device, **When** the
   catalog loads, **Then** no uncore row appears under the core device.
8. **Given** a device-scoped entry that refuses a per-task event,
   **When** the catalog publishes its availability, **Then** the
   published state separates that refusal from an encoding refusal, and
   a cpu-target plan over the entry compiles where the kernel grants the
   event.
9. **Given** a catalog entry, **When** a caller reads the targets it
   supports, **Then** the targets arrive as a fixed-size bitmask that
   allocates no memory, and adding a target kind to the enumeration
   leaves every existing value unchanged.

---

### User Story 5 - Install the package and find the catalog the build tree holds (Priority: P2)

A benchmark harness links the installed package. The vendored event
tables resolve through a build-time path into the source tree, and the
install rules publish no table data, so an installed package holds no
vendored rows.

The harness author installs the package, links it, and expects the same
vendored catalog the build tree publishes on the same host.

**Why this priority**: The harness ships as a package. A library that
carries no rows on the installed path makes every Intel and hybrid
correction above invisible to the consumer.

**Independent Test**: Install the package into a scratch prefix, run the
downstream consumer test against it, and compare the published
vendored-row count with the build tree's count on the same host.

**Acceptance Scenarios**:

1. **Given** a package installed into a scratch prefix, **When** a
   consumer links it and reads the catalog, **Then** the vendored-row
   count equals the build tree's count on the same host.
2. **Given** a consumer that relocates the installed tree, **When** it
   reads the catalog, **Then** the vendored rows resolve with the source
   tree absent.

---

### User Story 6 - Read a per-plan overhead floor that excludes the clock that measures it (Priority: P2)

A benchmark harness publishes a measurement floor beside every number it
reports, and the library publishes a per-plan overhead floor. The floor
is calibrated by bracketing each sampling action with two clock reads,
and the published figure carries part of the clock's own cost. No test
covers the calibration.

The harness author reads a floor that isolates the sampling action, and
trusts a figure a test exercises.

**Why this priority**: Every number the harness prints carries this
floor. A floor that overstates the cost makes every small benchmark look
worse than it is.

**Independent Test**: Run a calibration under a test registered with the
test runner and compare the published floor with the cost of the
bracketing clock reads measured under the identical bracketing. Spec 011
measured that bracket at 7.36 ns on the reference host.

**Acceptance Scenarios**:

1. **Given** a calibrated plan, **When** the plan publishes its overhead
   floor, **Then** the figure excludes the cost of the two clock reads
   that bracket each sampling action.
2. **Given** the calibration code, **When** the test suite runs,
   **Then** a registered test exercises the calibration and fails when
   the correction is absent.

---

### User Story 7 - Read a clock leaf whose documented order the platform keeps (Priority: P2)

A benchmark harness compares and merges timestamps taken on different
worker threads. The clock class documents one order guarantee for every
leaf, and two leaves cannot keep it: one leaf reads a clock that runs
per thread, and one leaf reads the processor's cycle counter with no
ordering fence.

The harness author reads a per-leaf guarantee the platform keeps, with a
test for each stated guarantee.

**Why this priority**: A contract the code does not enforce is a defect
under Principle II. The harness relies on the stated order when it
merges timestamps across threads.

**Independent Test**: Sample `machine/thread_cpu` on a new thread and
compare the value with a sample taken earlier on another thread. Read
each leaf's documented guarantee and match a test to each one.

**Acceptance Scenarios**:

1. **Given** a sample taken on one thread, **When** a new thread takes
   its first sample of the per-thread CPU clock, **Then** the new value
   can fall below the earlier value, and the leaf's documented guarantee
   permits that result.
2. **Given** the timestamp-counter leaf, **When** a reader checks its
   documented guarantee, **Then** the guarantee states the precondition
   its justification assumes, which is that one thread takes both window
   endpoints.
3. **Given** the clock class, **When** a reader reads its contract,
   **Then** each leaf states its own guarantee, and a test exercises
   each stated guarantee.

---

### Edge Cases

- **Host capability set, entry without it**: The host grants the fast
  instruction to some events and refuses others. The catalog publishes
  the verdict per entry, so one refusal never changes another entry's
  mode.
- **Counter value crossing the width boundary**: A run of several hours
  advances the counter past the value its width can hold. The published
  point is a 64-bit cumulative count, so the fold stays correct.
- **Refused read on the first attempt and on the retry**: Both reach the
  stated fallback, and neither keeps the value of an earlier read.
- **Group read short by one word**: The leader read returns fewer bytes
  than the header. No point reaches the sink that a fold accepts as a
  count.
- **Group read refused outright**: The syscall reports an error and no
  page is filled. The sampling action discloses the failure through the
  plan's disclosure column.
- **Partial window open**: A fast window opens three of four members and
  the fourth fails. The three acquired descriptors and mappings are
  released, and no window is published.
- **Kernel refuses the event under test**: The lifecycle test cannot
  open a real event. It opens a substituted descriptor instead and
  counts `/proc/self/fd`, and the real-resource measurement runs where
  the kernel grants the event.
- **Plan destroyed while never opened**: The destruction releases
  nothing and fails no check.
- **Two threads resolving one canonical address**: Both insert into the
  same handle map today. Both must return the published entry.
- **Compile racing a catalog read**: The compile writes the open flag on
  every call today. The write has no place after the catalog is open.
- **Intel sampling keys**: `SampleAfterValue`, `MSRValue`, `MSRIndex`
, `CounterMask`, `Invert`, `EdgeDetect`, `PEBS`, `Data_LA`,
  `PerPkg`, and `Experimental` name no kernel format. Each carries no
  encoding obligation.
- **AMD rows that already encode**: The parse rule covers both families.
  The change that lets Intel rows encode must not lower the AMD counts.
- **Hybrid host device names**: The host publishes core and atom
  devices. A core-scoped row reaches each applicable core device, and
  the uncore rows stay on the uncore devices.
- **Row scoped to an absent device**: The host publishes no device the
  row names. The row stays out of the catalog and no probe runs for it.
- **Device-scoped entry under a per-task probe**: Uncore and power
  entries refuse a per-task event. The catalog records the scope
  refusal, and a cpu-target plan compiles over the entry where the
  kernel grants the cpu-targeted event.
- **A kernel target the bitmask has no bit for**: The enumeration of
  target kinds grows a bit when the kernel publishes a new target. No
  enumeration value changes, and no stored bit moves.
- **Calibration floor smaller than the clock read**: A plan whose
  sampling action costs less than one clock read still publishes a
  floor. The corrected figure can reach zero on a host whose clock is
  slow, and the published figure then names the condition.
- **New thread against an earlier sample**: The per-thread CPU clock of
  a new thread starts near zero, and an earlier sample on another thread
  can hold a large value. The documented guarantee permits the result.
- **Timestamp counter across processors**: Two reads from different
  processors carry no documented order. The leaf's guarantee states the
  precondition that one thread takes both endpoints, adds no fence, and
  claims no order of its own (FR-031).
- **Cpu-target plan read by a thread on another processor**: The fast
  read path binds a context to the processor it opened on. The sampling
  thread has to stay on its target, and the semantic-gated precondition
  states it, so a dev or CI build reports the migrated thread and a
  release build configured `ignore` checks nothing (FR-045).
- **Suspected defect that does not reproduce**: The defect leaves the
  scope, and the plan records the host, the run, and the evidence.
- **Coverage exclusions and analyzer findings**: The touched lines add
  no coverage-exclusion marker, and the analyzer finding count of each
  touched translation unit does not rise.

## Requirements *(mandatory)*

### Fast-path reads and folds (I-04, I-05)

- **FR-002**: A fast read the page refuses SHALL disclose a gap through
  the plan's disclosure column, one managed column the sampling action
  writes beside the counts, and SHALL NOT retain the value of an earlier
  read (007 FR-040). The stated fallback SHALL NOT issue a syscall read
  of the same event, so the sampling action keeps the cost this library
  publishes for it. The column carries the countability value the
  catalog publishes for the entry on a measured action, and the
  availability state's gap value on an action that measured nothing
  (FR-007).
- **FR-003**: A fast read whose page reports a moving sequence after the
  stated retry SHALL disclose a gap through that same disclosure column,
  and SHALL NOT publish the value read before the sequence moved (007
  FR-040). The column carries the countability value the catalog
  publishes for the entry on a measured action, and the availability
  state's gap value on an action that measured nothing (FR-007).
- **FR-004**: The library SHALL decode a fast read into a cumulative
  64-bit count, following the recipe the kernel's own interface header
  documents, so that the point does not wrap at the published counter
  width and the fold's modular delta stays correct (007 FR-013).
- **FR-005**: The library SHALL compute the enabled and running pair of
  a fast window with the time computation the kernel's interface header
  documents, including the extrapolation that header names (007 FR-019,
  FR-041). The requirement names the computation by its content, the
  scale, offset, and shift fields the kernel's own interface header
  documents, so no file path or line number in any kernel header is
  part of this requirement.
- **FR-006**: A group read that returns fewer bytes than the group
  header SHALL mark that action in the plan's disclosure column, so no
  fold the caller performs over the marked action reports a delta from a
  count the read never produced (007 FR-011). This is the short-read
  case of FR-007, and it discloses through the same column with the
  same value.
- **FR-007**: The library SHALL disclose a failed group read through the
  plan's disclosure column, and SHALL NOT add per-sample metadata for
  that disclosure (007 FR-026). The disclosure column SHALL be one
  managed column written through the existing `point_sink::put`, the
  mechanism the `enabled` and `running` ratio pair already uses, so
  `put` keeps its one-integer signature and 007 FR-011 holds. On a
  measured action the column SHALL carry the countability value the
  catalog publishes for the entry, which is the `availability` field of
  `catalog_entry` in `include/speedgun-ng/counters_core.hpp`. On an
  action that measured nothing the column SHALL carry the availability
  state's gap value, `availability::gap`. The catalog SHALL NOT publish
  the gap value for an entry. The disclosure column is one managed
  column per sampling action, resolved in the same compile-time pass
  that resolves the ratio pair, and written after the counts and the
  ratio pair's two columns.
- **FR-008**: `recorder::sample()` SHALL remain `noexcept`,
  allocation-free, and lock-free after this feature, and a registered
  test SHALL assert all three with no recorded constant, because a CI
  runner can decide them (007 FR-026, Principle VII). Its release-build
  median cost on the reference host SHALL stay within 5 percent of the
  figure measured at the pre-fix head, measured as the median over 64
  repeats of 1000 actions at a fixed iteration count on a pinned
  processor, and that bound SHALL be a recorded measurement with no
  CTest test asserting it. Two plans are gated: the core PMU group over
  `cpu/instructions` and `cpu/cpu-cycles` with one read per leader per
  action, and the clock-leaf plan over `machine/monotonic` beside it.
  The plan SHALL measure each gated plan's median at `6aafd2d` and again
  after the correction, on the reference host, under one run method
  across both measurements, and SHALL record both figures and that run
  method. Each measurement SHALL be the median over repeated runs at a
  fixed iteration count on a pinned processor, because a single run on
  a shared host moves further than that tolerance. The measurement
  SHALL cover the disclosure column FR-007 adds as well.

### Recorder lifetime

- **FR-009**: This feature SHALL NOT add, change, or remove a recorder
  reuse, reset, or arena-lifetime entry point. Reuse of one recorder
  across repetitions belongs to the harness specification, which holds
  the repetition model. The shipped behaviour stays as it is: a recorder
  arena lives for the lifetime of its plan, every `plan::recorder()`
  call allocates a new arena, and a recorder has no reset.

### Catalog and plan concurrency (I-06)

- **FR-010**: Reading the catalog after open SHALL be safe from any
  number of threads at once. That covers resolving an object, listing
  objects, and reading an object's parent and children (007 FR-009,
  FR-031).
- **FR-011**: Compiling a plan SHALL NOT write shared state after the
  catalog is open, and SHALL be safe from any number of threads at once
  (007 FR-031).
- **FR-012**: A thread-sanitizer run in which several threads resolve
  counters and compile plans concurrently after open SHALL report no
  race. A preset SHALL configure the thread sanitizer, and a CI job
  SHALL build the suite under that preset and run it, so the gate cannot
  pass on a run that never happened (Principle VIII). The preset SHALL
  carry either the thread-sanitizer flags on the vendored trees or a
  suppressions file under version control, and the file's contents are
  the only accepted suppression. The thread-sanitizer preset SHALL stay
  separate from the address and undefined-behavior sanitizer preset,
  and adding the thread sanitizer to that preset SHALL NOT be permitted,
  because the compilers reject the pairing.

### Fast-window resource lifetime (I-08)

- **FR-013**: Destroying a plan SHALL release every event descriptor and
  every mapping the plan acquired (007 FR-040, FR-041).
- **FR-014**: A fast window whose later member fails to open SHALL
  release the descriptors and mappings the earlier members acquired, and
  SHALL publish no partial window.
- **FR-015**: A plan destroyed before it was opened SHALL release
  nothing and SHALL fail no check.

### Event tables, devices, and availability (I-01, I-02, I-03)

- **FR-001**: The catalog SHALL publish the fast read mode on an entry
  only where the fast read can succeed for that entry. A host-wide
  capability verdict SHALL NOT set the mode on an entry whose event the
  fast instruction cannot read. One probe serves this requirement and
  FR-022, because both read the same verdict at the same site.
- **FR-016**: A sampling key or a metadata key in an event table SHALL
  carry no encoding obligation. The table parser SHALL record a numeric
  key as an encoding field only where the key names a format the running
  kernel publishes.
- **FR-017**: An encoding key SHALL reach the kernel format it names,
  including the keys the kernel spells `cmask`, `inv`, `edge`, and
  `offcore_rsp`.
- **FR-018**: A row SHALL be published as `not_encodable` only where the
  running kernel's formats lack a field the row needs (007 FR-037). A
  registered test SHALL assert both arms of this requirement: a row
  that needs a field the running kernel does not publish reports
  `not_encodable`, and a row whose fields the kernel publishes never
  reports it.
- **FR-019**: The catalog SHALL publish each vendored row on the device
  its table scope names. A core-scoped row SHALL appear on each core
  device the scope applies to, a row scoped to an absent device SHALL
  stay out of the catalog, and no uncore row SHALL appear under the core
  device.
- **FR-020**: The library SHALL publish the encodable-row count over the
  pinned table tree for each Intel core architecture directory. The plan
  SHALL measure each count over the pinned tree after the correction and
  record it, and a fixture test SHALL pin those exact numbers for
  `skylake`, `icelake`, `alderlake`, and `sapphirerapids`. Each count
  SHALL be measured against the named synthetic sysfs format list the
  fixture supplies, so one number gates every host on the matrix, and
  the plan SHALL record the count the reference host's own formats
  yield beside it (FR-034). The counts for `amdzen4` and `amdzen5` SHALL
  NOT fall, and a count of one row SHALL NOT satisfy this requirement.
- **FR-021**: The catalog's availability SHALL reflect the targets an
  entry can be counted on, and SHALL distinguish a refusal that comes
  from the entry's scope from a refusal that comes from its encoding
  (007 FR-024). The countability state SHALL be one enumeration whose
  values separate those two refusal causes, and the supported target
  kinds SHALL travel in the fixed-size bitmask field beside it, both
  on `catalog_entry` in `include/speedgun-ng/counters_core.hpp`.
  `catalog_seed` in `include/speedgun-ng/counters_provider.hpp` is the
  provider's seeding surface and gains no field. The bitmask SHALL be a
  fixed-size type that allocates no memory, so a caller reads the
  targets without a container. The availability state gains one value
  naming the per-action gap case, which the catalog never publishes
  for an entry and which the plan's disclosure column carries for each
  sampling action (FR-007).
- **FR-022**: A cpu-target plan SHALL compile over an entry whose scope
  refuses a per-task event, where the kernel grants the cpu-targeted
  event (007 FR-024, FR-031).

### Installed package (I-07)

- **FR-023**: The build SHALL embed the vendored event tables into the
  library archive as the raw JSON bytes held in static data, and the
  library's existing simdjson parse path SHALL decode those bytes at run
  time, so that an installed package publishes the same vendored catalog
  as the build tree on the same host. The library SHALL reach the tables
  with no run-time path lookup, and SHALL add no configured data path,
  no new codec, no new dependency, and no public surface for any of
  them
  (007 FR-050, SC-008). The embedding SHALL NOT be conditional on a
  build option. The plan SHALL record the size this adds to the
  archive and to each executable that links the library, measured on the
  reference host (Principle VII).
- **FR-024**: This feature SHALL ship as version 0.3.0, and `SOVERSION`
  SHALL stay at 0. FR-021 adds public surface, a countability value that
  separates a scope refusal from an encoding refusal beside a fixed-size
  bitmask of supported target kinds, and changes no existing signature,
  so the change is additive and takes the minor bump under the
  precedent spec 008 set. FR-007 widens no signature: the disclosure
  travels in a managed column written through the existing
  `point_sink::put`. The plan records the bump in the version lineage
  (Refactoring and Evolution) and records why the patch-level wording in
  spec 011 does not extend to a field added to a public record.

### Overhead floor (I-09)

- **FR-025**: The published per-plan overhead floor SHALL isolate the
  sampling action from the clock reads that bracket it, on the terms
  spec 011 established for its own per-read figure (011 R-006).
- **FR-026**: A test registered with the test runner SHALL cover the
  calibration, and no coverage exclusion SHALL hide the calibration from
  that test.
- **FR-027**: The count of coverage-exclusion markers in
  `source/counters/` SHALL NOT rise over this feature. This feature
  removes the markers that stand on the paths it corrects and adds
  none (FR-046).

### Clock order contract (I-10)

- **FR-028**: Each clock leaf SHALL document its own order guarantee,
  separately from every other leaf, and the class contract SHALL state
  no order on a leaf's behalf.
- **FR-029**: Every order guarantee a leaf documents SHALL have a test
  that exercises it.
- **FR-030**: No clock leaf SHALL document an order its clock does not
  provide. The per-thread CPU clock's documented guarantee SHALL permit
  a new thread's sample to fall below an earlier sample taken on another
  thread (011 FR-006, FR-007).
- **FR-031**: The timestamp-counter leaf's documented guarantee SHALL
  state the precondition its justification assumes, which is that one
  thread takes both window endpoints. The leaf SHALL gain no ordering
  fence, so its per-read cost does not rise. The guarantee a caller
  reads is a precondition on the caller. The clock delivers no order
  across threads.

### Verification, traceability, and repository constraints

- **FR-032**: Every fix in this feature SHALL ship with a test that
  fails at `6aafd2d` and passes after the fix (Principle VI, X.4).
- **FR-033**: The plan for this feature SHALL record TDD mode (Principle
  III).
- **FR-034**: Every test in this feature SHALL run unprivileged on the
  CI matrix at `perf_event_paranoid` 2. A test that involves the kernel
  SHALL use synthetic sysfs, table, and event-page inputs (007 SC-002).
- **FR-035**: The counters classes SHALL stay embeddable in
  fixed-iteration, per-thread benchmark loops, and no
  benchmarking-framework code SHALL appear in the library (007 FR-049,
  FR-050).
- **FR-036**: No public header SHALL gain a platform term (
  `test/counters_header_purity.sh`).
- **FR-037**: Every changed interface SHALL keep its doxygen contract
  paired with a registered enforcement counterpart (`dbc-gate`,
  Principle II).
- **FR-038**: Every correction in this feature SHALL be recorded in
  `specs/007-counters-and-timers/citations-log.md` as an entry in that
  file's format, naming the 007, 008, or 011 requirement it restores.
- **FR-039**: A suspected defect that does not reproduce on the affected
  hardware SHALL leave the scope, and the plan SHALL record the host,
  the run, and the evidence.
- **FR-040**: The code this feature adds or changes SHALL reach 100
  percent line coverage, 100 percent branch coverage, and 100 percent
  contract coverage under the project's existing gates.
- **FR-041**: The clang-tidy warning count of each touched translation
  unit SHALL NOT rise over this feature.
- **FR-042**: The pre-existing clang-tidy and coverage-exclusion backlog
  outside the lines this feature touches SHALL stay untouched, and every
  changed line SHALL trace to a requirement in this specification
  (Principle X.3). The coverage-exclusion half of this requirement is
  the general rule, and FR-027 states its `source/counters/` instance.
- **FR-043**: This feature SHALL NOT add an event source, an object
  kind, a provider, or a public header that no requirement above names.
- **FR-044**: All prose this feature adds SHALL satisfy Principle XI,
  including XI.7.
- **FR-045**: A cpu-target fast-mode plan SHALL require its sampling
  thread to run on the processor the plan opened its contexts on, and
  the library SHALL state that requirement as a semantic-gated
  precondition paired with its doxygen contract (Principle II). A
  sampling thread that has migrated SHALL be caught by `SG_REQUIRE` in
  every configuration that emits contract code, and a release build
  configured `ignore` SHALL emit no such check. The requirement adds no
  branch to `recorder::sample()`, so the per-sample cost stays where
  FR-008 holds it. A registered test SHALL drive the migrated-thread
  path in a contract-emitting configuration and observe the abort,
  and a second check SHALL confirm that a build configured `ignore`
  emits no such check.
- **FR-046**: Every corrected decision on a path the kernel alone can
  execute SHALL be extracted into a small pure function declared in
  the project's existing private seam header. That function SHALL
  carry a doxygen clause and a paired enforcement site, and a
  registered test SHALL drive it over synthetic input. The corrected
  decision SHALL reach the coverage gates through that function. A
  coverage-exclusion marker standing on the kernel-facing wrapper
  that a granted event alone can enter SHALL stay in place, and the
  coverage-exclusion marker standing on a release arm of a corrected
  decision SHALL go (Principle VI, X.2, X.3).

### Key Entities

- **Defect record**: One entry per issue, holding a stable identifier (
  `I-01` through `I-10`), a priority (P1 or P2), a status (confirmed or
  suspected), the source sites that show the defect, and the merged
  requirement the correction restores.
- **Fast-path point**: The value one sampling action hands to the sink:
  a cumulative 64-bit count in one managed column, beside the ratio
  pair's two columns and beside the plan's one disclosure column, which
  carry the multiplex ratio and the countability state of that action.
- **Availability state**: What the catalog publishes for one entry: one
  countability value that separates a refusal caused by the entry's
  scope from a refusal caused by its encoding, and beside it a
  fixed-size bitmask naming the target kinds the entry can be counted
  on.
  The bitmask allocates no memory. The plan's disclosure column carries
  the same countability value for each sampling action.
- **Event table row**: One event selector with its encoding fields, its
  device scope, and the architecture directory its table came from.
- **Per-plan overhead floor**: The published minimum and median
  nanoseconds a plan reports for one sampling action, with the
  bracketing clock reads excluded.
- **Clock leaf order guarantee**: The ordering one leaf documents for
  its own platform clock, with a test for each stated guarantee.
- **Successor-log entry**: A dated record in the frozen record's
  successor, naming the correction, the requirement it restores, the
  command that measured it, and the head it was measured at.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Page fixtures prove three results. A point that crosses
  the published counter width folds to the true delta. A refused read
  takes the stated fallback. The ratio over a multiplexed window matches
  the kernel's own time recipe to within one nanosecond.
- **SC-002**: A fixture that fails one group read produces no fold
  result whose delta exceeds the counts the fixture drove.
- **SC-003**: A thread-sanitizer run reports no race while several
  threads resolve counters and compile plans concurrently after open.
  A CI job builds the suite under the thread-sanitizer preset and runs
  it, and the preset resolves the uninstrumented vendored code with
  either matching flags or a version-controlled suppressions file.
- **SC-004**: Opening and destroying a fast-mode plan 10,000 times
  leaves the process descriptor count and mapping count at their
  starting values. One test proves this on every job, over substituted
  descriptors the library opens and closes through its own path. A
  second test proves it against real event descriptors and mappings on a
  host whose kernel grants the event, and reports that it could not run
  elsewhere.
- **SC-005**: The library publishes the encodable-row count over the
  pinned table tree for each Intel core architecture directory, measured
  against the named synthetic format list the fixture supplies. Every
  Intel row whose fields map to published kernel formats encodes. The
  plan records the count measured over the pinned tree after the
  correction, a fixture pins that exact number for `skylake`, `icelake`,
  `alderlake`, and `sapphirerapids`, the plan records the count the
  reference host's own formats yield beside it, and the counts for
  `amdzen4` and `amdzen5` do not fall.
- **SC-006**: Synthetic hybrid and uncore device fixtures place every
  row on the device its scope names, and no uncore row appears under the
  core device.
- **SC-007**: A fixture with a device-scoped unit reports a state that
  separates a scope refusal from an encoding refusal, and a cpu-target
  plan over that entry compiles where the kernel grants it. The
  supported target kinds arrive as a fixed-size bitmask that allocates
  no memory.
- **SC-008**: The downstream consumer test runs against an installed
  package and publishes a vendored-row count equal to the build tree's
  count on the same host.
- **SC-009**: A test registered with the test runner covers the
  calibration, and the published floor for a plan excludes the cost of
  the bracketing clock reads.
- **SC-010**: Each clock leaf documents its own order guarantee. A test
  shows a new thread's per-thread CPU clock sample below an earlier
  sample taken on another thread, and the documented guarantee permits
  that result.
- **SC-011**: `ctest`, `dbc-gate`, `format-check`, `spell-check`,
  `prose-lint`, the sanitizer presets, the thread-sanitizer preset and
  its job, and the 100 percent line, branch,
  and contract coverage gates pass. The coverage-exclusion marker count
  in `source/counters/` does not rise, and the clang-tidy warning count
  of each touched translation unit does not rise.
- **SC-012**: The plan records the archive size and the linked
  executable size with the tables embedded, measured on the reference
  host, and the figure names the cost every consumer of the installed
  package carries.

## Assumptions

- Every value the library hands a caller is a correct value, or the
  library discloses that the value is unavailable. No third state ships.
- Two tests cover the resource lifetime of a fast-mode plan. The
  everywhere-runnable test counts real descriptors through
  `/proc/self/fd` and `/proc/self/maps` over resources that stand in for
  event descriptors, so it measures release and not a counter. The
  host-dependent test measures the real event descriptors and mappings
  where the kernel grants the event, and reports that it could not run
  elsewhere (SC-004).
- A defect marked confirmed rests on the cited lines alone. A defect
  marked suspected rests on a reading the code supports, and clarify or
  plan confirms it on the affected hardware.
- The reference host is AMD. Spec 011 names it as an AMD Ryzen 9 9950X3D
  running Linux 7.2.4-1-cachyos with glibc 2.44, and the same host
  serves every figure this specification publishes.
- The CI matrix runs at `perf_event_paranoid` 2 on Linux on GCC and
  Clang, where no test may need a privileged event the kernel grants by
  default.
- The concurrency defect of I-06 is settled by a thread-sanitizer run
  under its own preset and its own CI job. The address and
  undefined-behavior preset keeps its present sanitizers, and the
  compilers reject the pairing that would merge the two presets
  (FR-012).
- A test that needs kernel behaviour uses synthetic sysfs, table, and
  event-page inputs, as spec 007 required in its SC-002.
- The kernel's own interface header is the authority for the fast-read
  decode recipe and for the enabled and running time computation. Where
  the header and the tree disagree, the header governs.
- The vendored event tables stay pinned to the recorded kernel tag, and
  this feature re-pins nothing.
- The counters library keeps zero external runtime dependencies, and
  this feature adds none.
- The successor log at `specs/007-counters-and-timers/citations-log.md`
  keeps accepting corrections in its recorded entry format, and the
  frozen record takes no edit.
- The benchmark harness, its registration API, and its reporting belong
  to the next specification. This feature corrects the substrate the
  harness compiles plans against.
- Recorder arenas live for the lifetime of their plan, every
  `plan::recorder()` call allocates a new arena, and a recorder has no
  reset. Reuse of one recorder across repetitions belongs to the harness
  specification, and FR-009 holds this feature to no recorder change.
- The installed library is an archive whose code links into the
  consumer's executable. The archive has no location at run time, so the
  vendored tables travel inside it as raw JSON bytes in static data and
  no run-time path resolves them (FR-023).
- The vendored tree holds 542 JSON files totaling 24.5 MB across 40
  architecture directories. Every executable that links the library
  carries that data, and the plan records the measured size under
  Principle VII (FR-023, SC-012).
- A cpu-target fast-mode plan opens its contexts on its target
  processor, and its sampling thread stays on that processor. The
  requirement is a semantic-gated precondition on the caller. It adds
  no run-time branch to the hot path, so a release build configured
  `ignore` emits no code for it and the per-sample cost stays where
  FR-008 holds it (FR-045).
- The timestamp-counter leaf reads with no ordering fence, and this
  feature adds none. A fence helps only a caller that reads across
  threads, and the harness takes both window endpoints on one thread, so
  a fence would cost a read per timestamp for no correctness the caller
  lacks (FR-031, Principle VII).
- Version lineage: the corrections in this feature add public API and
  widen no existing signature, so the release ships as the next minor
  release, 0.3.0, with `SOVERSION` staying at 0 (FR-024). Spec 008 took
  the minor for an additive change that added a member to `system` and
  changed no signature, and FR-021 has the same shape. Spec 011 called
  its purely additive change patch-level; the plan records why that
  wording does not extend to a field added beside the availability on a
  public record. Embedding the vendored tables into the archive changes
  no public surface. A correction that later removes public API ships a
  further deliberate bump the plan records.
- The availability a catalog entry publishes is a countability value
  beside a fixed-size bitmask of target kinds. The bitmask allocates no
  memory, so reading it costs no allocation on the harness's path
  (FR-021). The plan's disclosure column carries that same countability
  value per sampling action, written through the existing `put` beside
  the ratio pair, so no per-sample metadata is added and the sampling
  action keeps its published cost (FR-007).

## Out of Scope

- The benchmark harness, its registration API, and its reporting.
- Recorder reuse across repetitions, a recorder reset, and any change to
  a recorder's arena lifetime (FR-009).
- A configured path for the vendored event tables, and any public
  surface that would carry one (FR-023).
- A build option that turns the embedded tables off. A later
  specification adds one when a caller needs a smaller binary (FR-023).
- New event sources, including software events, cache events, and raw
  event strings.
- A new object kind, a topology tree, and metric-expression ingestion.
- Multiplex-aware group scheduling.
- Report serialization.
- A re-pin of the vendored event tables.
- Any change to the simulation-start marker that spec 011 added, apart
  from the clock contract in I-10.
- The pre-existing clang-tidy and coverage-exclusion backlog, except on
  the lines this feature touches.

## Open decisions for the plan

The audit's seven questions were all answered. Five were answered in
the Clarifications session of 2026-10-03: the table data placement, the
availability vocabulary with the refused-read fallback, the recorder
reuse, the embedded-data form with its unconditional build, and the
disclosure channel for a failed group read. Two were answered there
because their answer is a measurement. The descriptor and mapping count
under an unprivileged kernel, and the thread-sanitizer preset.

A second clarification session on 2026-10-04 settled the decisions this
specification had deferred, the version bump (FR-024), the cpu-target
pinning precondition (FR-045), the timestamp-counter ordering (FR-031),
the per-sample regression tolerance (FR-008), and the source of the
expected Intel encodable-row counts (FR-020, SC-005).

A third clarification session on 2026-10-04, a convergence pass,
answered eight further questions, and this section lists all eight
under the `### Session 2026-10-04 (convergence)` block. Two of them
changed a decision. The first gave the disclosure column a per-action
value, which FR-007 carries and FR-002 and FR-003 read. The second set
the coverage route for a corrected decision on a kernel-granted
path, which FR-046 names as a pure function in the project's existing
private seam header. The remaining six confirmed what the earlier
sessions settled.

No decision remains. The Intel confirmation of I-01 and I-04 on
hardware is recorded as deferred. The reference host is AMD, the
repository owner expects to add an Intel host later, and no requirement,
test, or gate in this specification depends on the confirmation. The
plan records the deferral, names the host class that would settle it,
and keeps the suspected parts of both defects in scope until that host
answers (FR-039).

Four figures the requirements name come from measurement. The plan
records each beside the requirement that names it: the two release-build
per-sample costs and their pre-fix measurements (FR-008), the
encodable-row counts over the named synthetic format list and over the
reference host's own formats (FR-020, SC-005), and the archive and
linked-executable sizes (FR-023, SC-012).

## Dependencies

- The kernel's own interface header, which documents the fast-read
  decode recipe and the enabled and running time computation. FR-004 and
  FR-005 take their content from it.
- The vendored event tables under `external/pmu-events`, pinned by the
  `RECORD` manifest. FR-016 through FR-020 read that tree, FR-023 embeds
  it, and no requirement changes the pin.
- The successor log at `specs/007-counters-and-timers/citations-log.md`
, which accepts every correction this feature records.
- The project's existing gates: the test runner, the thread-sanitizer
  preset, the coverage gates, the contract-pairing gate, the formatter
  at the pinned version, and the prose gate.
- The kernel running on each host, which grants or refuses each event
  the catalog probes. Where the kernel refuses, a fixture supplies the
  page.
- An Intel host, needed to confirm I-01 and I-04 on hardware. The
  confirmation is deferred, and the plan records the deferral and names
  the host class that would settle it. No requirement, test, or gate in
  this specification depends on it.