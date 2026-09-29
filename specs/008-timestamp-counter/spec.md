# Feature Specification: Raw Time-Stamp Counter

**Feature Branch**: `008-timestamp-counter`

**Created**: 2026-09-29

**Status**: Draft

**Input**: User description: "speedgun's tsc should map to __rdtsc()
in the implementation", "every cpu has a timestamp counter", "tsc is
a counter, like everything else we count in rdpmc", and "raw TSC,
nothing to calibrate, the read is a counter".

## What this feature adds

The CPU time-stamp counter is a counter, and a raw one.
`source/counters/clock_provider.cpp:146` reads it with a single
instruction, and that instruction returns a count. 007 already
declares the entry, FR-023 of 007 already mandates `fast_tsc` as a
read mode, and a counter in this library is reached through
`object::counter<D>(name)`, composes under the dimension algebra, and
folds with its disclosure attached. The time-stamp counter gets that
same treatment.

Today it gets a different one, and the difference is a calibration
that has no place on a count. `source/counters/clock_provider.cpp:206`
to :236 reads `/sys/devices/system/cpu/tsc_khz` and CPUID leaf 0x16 at
construction, and `:268` withholds the entry entirely unless that read
succeeds. Two consequences follow, and both are wrong for a counter.
A host that executes the instruction perfectly well and publishes no
frequency receives no entry at all, and a host that does publish one
receives an entry carrying a rate the library then invites callers to
reason about.

008 makes the entry raw. It publishes wherever the instruction
executes, which is a property of the build guard and needs no runtime
state, and it attaches no rate to the count.

## What this feature does not add

No reading type, no difference type, no conversion, no rate, no
calibration, no invariance status, and no capability query on the
provider. An earlier draft proposed a reading value carrying its own
unit, source, calibration, and invariance, a span type, a difference
operator, a nanosecond conversion, and a defaulted virtual on
`provider_iface` to reach the provider's private state. All of it is
withdrawn. A counter carrying a bespoke interface stops being
interchangeable with the others, and FR-019 of 007 already attaches
what a fold needs.

## User Scenarios & Testing

### User Story 1 - Count with the time-stamp counter on any host
(Priority: P1)

A developer counts events over a hot loop on a machine whose kernel
publishes no counter frequency. They resolve the time-stamp entry the
same way they resolve any other counter, compose it against a counted
event, and fold the ratio. The entry is there, because the instruction
is there.

**Why this priority**: It is the whole feature. The withheld entry is
the defect, and this story is the withholding removed.

**Independent Test**: On a host that executes the instruction, resolve
`machine` and ask for its `tsc` entry. The entry exists and reports
`read_mode::fast_tsc`. Passes on the development host with no fixture
and no configured machine.

**Acceptance Scenarios**:

1. **Given** a registered clock provider on a host executing the
   time-stamp instruction, **When** the caller enumerates the `machine`
   object, **Then** the catalog lists a `tsc` entry at
   `read_mode::fast_tsc` and reports it countable.
2. **Given** a host publishing no counter frequency, **When** the
   caller enumerates the same object, **Then** the entry is still
   listed, because publication follows the instruction and nothing
   else.
3. **Given** the listed entry, **When** the caller reads its
   description, **Then** the text describes a raw count and asserts no
   rate.

---

### User Story 2 - Reach the counter through one short name
(Priority: P2)

A developer wants the time-stamp counter and does not want to know
that it lives under `machine` or that its name is `tsc`. They call
`system::local().tsc()` and receive a value they can feed straight
into `compile()` alongside any other counter.

**Why this priority**: It is the spelling the owner asked for twice.
It returns the identical type the uniform lookup returns, so it
changes no capability and it composes with everything else.

**Independent Test**: Call `system::local().tsc()` and compare it
against `machine->counter<events>("tsc")`. Both name the same entry and
both accept the same expressions. Passes on its own, independently of
the publication change.

**Acceptance Scenarios**:

1. **Given** a clock provider is registered, **When** the caller
   invokes `system::local().tsc()`, **Then** the call returns the same
   counter type that resolving the entry by name returns, naming the
   same canonical entry.
2. **Given** no clock provider is registered, **When** the caller
   invokes it, **Then** the call returns a recoverable error naming the
   absent provider, and the program may still register one afterwards.
3. **Given** a host that does not execute the instruction, **When** the
   caller invokes it, **Then** the call returns a recoverable
   not-present error naming the absent counter, and performs no read.

---

### User Story 3 - Time the library's own sampling path from inside
the library (Priority: P3)

A contributor measures the cost of a fold. That measurement code lives
in this repository and links against the library, yet it reaches for
the standard library's clock because the library exposed no counter to
reach. With the entry published, the overhead benchmark brackets its own
fold loop with a library counter.

The published time-stamp entry cannot serve that bracket, and the
reason belongs in the record. It carries a count and no rate, so it
yields no nanoseconds, and the library attaches no frequency that would
let a caller convert one into the other. The `machine/monotonic` leaf
is the one library counter carrying a duration, so it brackets. The
sampling action already used the library's own accessors.

**Why this priority**: It closes the artifact that exposed the defect,
and it proves the counter composes in the library's own tooling.

**Independent Test**: Replace the standard-library clock in the
overhead benchmark with a library counter, rebuild, and confirm the
figure the library counter produces matches the figure the standard
library produces for the same loop. Passes on its own once the entry is
published.

**Acceptance Scenarios**:

1. **Given** the overhead benchmark, **When** it brackets its fold loop
   with the library's monotonic counter and again with the standard
   library, **Then** the two reported per-fold costs agree to within
   1 ns.
2. **Given** a program holding an ambient lookup result and a recorded
   leaf, **When** the caller composes them in one expression, **Then**
   the plan compiles and folds with full disclosure, because both are
   counters of the same shape.

---

### Edge Cases

- **No clock provider registered.** The lookup returns a recoverable
  error naming the absent provider. It does not open the registration
  boundary, so the caller may still register afterwards.
- **A host without the instruction.** The entry is absent from the
  catalog, so the lookup returns a recoverable not-present error naming
  the absent counter. This is the only platform-conditional behaviour
  the feature has, and it is decided at build time.
- **Two threads.** Each plan and each recorder binds to its calling
  thread. Two threads counting themselves neither contend nor observe
  each other.
- **A delta of zero.** A sampling action that retires nothing yields a
  zero delta, which is a valid count and carries its unit, exactly as a
  zero on any counted source does.
- **A counter that resets.** The count is cumulative and wraps like any
  other 64-bit cumulative counter in the library. This feature asserts
  no epoch and no monotonicity across resets.

## Requirements

### Functional Requirements

- **FR-001**: The clock provider shall publish the time-stamp catalog
  entry wherever the build executes the time-stamp instruction, and
  shall decide that condition at build time with no runtime state and no
  file read.
- **FR-002**: The entry shall carry a count and no rate. Its unit shall
  remain the closed token 007 assigns it, its frequency field shall
  carry the zero default, and its description shall assert no rate.
- **FR-003**: The provider shall open a reader for the time-stamp entry
  exactly where it enumerates that entry, so the catalog and the reader
  agree on one condition.
- **FR-004**: The counters system shall expose the time-stamp entry as
  `system::local().tsc()`, returning the same counter type that
  resolving the entry by name under the `machine` object returns.
- **FR-005**: The lookup shall perform no hardware read, construct no
  plan, and mint no recorder.
- **FR-006**: The lookup shall not open the registration boundary, so a
  program may call it and then register a provider.
- **FR-007**: Where no clock provider is registered, the lookup shall
  return a recoverable error naming the absent provider, and shall
  return no counter.
- **FR-008**: Where the build does not execute the instruction, the
  lookup shall return a recoverable not-present error naming the absent
  counter, and shall perform no read.
- **FR-009**: Each new or changed public function shall carry a
  doxygen contract stating its precondition and its postcondition, and
  shall pair every documented precondition with a runtime check, so the
  `dbc-gate` target pairs documentation with enforcement.
- **FR-010**: The public core vocabulary of this feature shall add no
  new token. `tsc` is already required vocabulary, because FR-034 of
  007 declares a `tsc` entry and FR-023 of 007 mandates `fast_tsc` as a
  read-mode name, and `test/counters_header_purity.sh` declines to scan
  `tsc` on that ground. The accessor name is the same token.
- **FR-011**: The provider shall no longer read a counter frequency, no
  longer read the CPUID nominal-frequency leaf, and no longer populate a
  frequency or a scaled flag for this entry, and the change shall be
  recorded as superseding the calibration and publication obligations of
  FR-034 of 007 for this counter.

### Key Entities

No entity is added and no field changes. The feature publishes an entry
007 already declares and stops populating two of its fields, and it
adds one accessor returning the counter type 007 already defines.

### Success Criteria

**Measurable Outcomes**:

- **SC-001**: On a host that executes the instruction, the catalog lists
  the time-stamp entry and a program can resolve it, compose it against
  another counter, and fold the ratio, with no fixture and no
  compile-time switch.
- **SC-002**: A program reaches the counter by one call and feeds the
  result straight into an expression beside any other counter, with no
  new type to learn.
- **SC-003**: The entry attaches no rate, and no new surface associates
  elapsed time with a count, so a caller reading the entry learns a
  count and nothing further.
- **SC-004**: In the platform release preset, a sampling action over a
  plan holding only the time-stamp entry costs at most 30 ns at the
  median, and the measured figure is published in the counters overhead
  page beside the read's own tick distribution.
- **SC-005**: The existing overhead benchmark brackets its fold loop
  both ways, with the library's own counter and with the standard
  library, and the two published figures agree to within 1 ns, so the
  library's counter measures the same quantity the standard library
  does.
- **SC-006**: The header purity gate, the contract pairing gate, and
  the full test suite pass, and the Linux CI matrix stays green. No
  macOS or Windows job exists in the workflow, so this criterion names
  no platform the repository does not build.

## Assumptions

- The instruction is assumed present wherever the build enables the
  x86 guard, since x86-64 mandates it. A host without it reaches the
  not-present path of FR-008.
- The count is raw. No rate, no calibration, and no conversion of a
  count into elapsed time ship here, and 007 already recorded that
  deferral. This feature counts.
- A caller wanting a rate supplies it from outside the library. The
  library declines to attach one, so no folded value implies a duration
  that the count alone does not carry.
- The feature is additive at version 0.1.0. It adds one member to
  `system`, changes one publication condition, stops populating two
  fields on one entry, and changes no existing signature, so `SOVERSION`
  stays at 0 and the change ships as 0.2.0. The constitution requires a
  deliberate version decision on a public API change, and this records
  it.
- Latency histograms, string-formula metrics, the benchmark harness,
  report serialization, and non-x86 hardware PMU providers stay out of
  scope, as 007 recorded them.
- This feature corrects 007 in place. The merged text is not rewritten;
  the correction is recorded here and in the successor log, which is the
  mechanism 007 established for exactly this case.
