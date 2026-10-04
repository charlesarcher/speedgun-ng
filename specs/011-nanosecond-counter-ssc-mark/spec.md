# Feature Specification: Nanosecond Counter and Simulation-Start Marker

**Feature Branch**: `011-nanosecond-counter-ssc-mark`

**Created**: 2026-10-03

**Status**: Draft

**Input**: User description, quoted verbatim: <!-- prose-lint: allow reason="XI.5 verbatim quotation of the request, marked as quoted" --> "I want to add two new "counters", one is a nanosecond counter (implemented with the fastest non syscall clock gettime) counter. And I want to add another, new api called simulation start based on intel's "TRACING_SSC_MARK" macro. The nanosecond counter should just be a counter like every other one, but the tracing ssc mark is meant to open and signal an emulator like Intel SDE to start tracing instructions. This one should be a new API, ideally it won't be macro based but maybe it has to be, given the immediate instruction."

## Purpose

Two additions to the library's measurement surface:

- A nanosecond-unit clock counter on the fastest clock path the platform
  offers without a system call.
- A public API named `simulation_start` that emits the Intel SDE SSC
  marker instruction sequence, telling an attached tracer to begin
  collecting instructions at this point in the run.

## Clarifications

### Session 2026-10-03

- Q: What canonical name does the new clock counter carry, the string
  a caller types to look it up? → A: `machine/monotonic_raw`
- Q: Which numeric tag value does the trace-start marker carry, the
  number a caller passes to Intel SDE on the command line? → A: `0xFACE`
- Q: Does the catalog keep labeling this counter's read mode as
  `syscall`, when the read avoids the system call? → A: Reuse
  `syscall`, matching every existing clock counter, and keep the
  fast-path caveat in the overhead table

### Session 2026-10-03 (closing pass)

- Q: Under what condition does a sample compare against a sample taken on
  another thread? → A: Only when the earlier sample's completion
  happens-before the later sample's start. The kernel clamps a detected
  backwards hardware-counter jump for the reader that observed it, and
  that clamp establishes no order between two processors.
- Q: Which measurement convention do SC-004's thresholds judge? → A: One
  read bracketed by two timestamp-counter reads, with the bracketing
  overhead measured under the identical bracketing and reported beside
  the figure. The threshold judges the overhead-corrected figure, and
  the uncorrected figure is published next to it.
- Q: Where does SC-006's sixty-second rate comparison run? → A: As a local
  confirmation recorded in the feature's quickstart, never as a CTest
  entry, because Principle VI requires tests fast enough to run in
  every CI job.
- Q: Which Intel identifier is the marker named from? → A: `__SSC_MARK`, an
  Intel C++ Compiler builtin mirrored in Clang's own intrinsic header.
  Intel publishes no macro named `TRACING_SSC_MARK` and publishes no
  byte encoding, so the byte contract rests on that macro together with
  the byte-level match in Intel's own tracing component.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Open an instruction trace at a chosen point (Priority: P1)

A performance engineer profiles a long benchmark run under Intel SDE.
SDE collects from process start, which floods the trace with setup,
fixture construction, and warm-up iterations the engineer does not care
about. The engineer wants collection to begin at the first measured
iteration, so the trace holds only interesting instructions.

The engineer writes one call at the top of the measured region, runs the
binary under `sde64` with the documented tag on the command line, and
reads a trace that begins where the call was placed.

**Why this priority**: This capability exists nowhere in the project
today. No SDE marker, no trace-control API, and no
emulator-instrumentation vocabulary appears in any first-party file. It
is greenfield, and it delivers a capability the library cannot already
provide.

**Independent Test**: Place one call in an existing example benchmark,
build, run the binary under Intel SDE with the documented tag, and
confirm the trace output begins at the marked region. The marker
assertion runs in the ordinary test suite with no tracer present; the
SDE run is a local confirmation.

**Acceptance Scenarios**:

1. **Given** a benchmark binary linked against the library and an
   attached Intel SDE, **When** the measured region begins with a
   `simulation_start` call, **Then** SDE begins collecting instructions
   at that point.
2. **Given** the same binary run with no tracer attached, **When** the
   measured region begins with a `simulation_start` call, **Then**
   execution continues and every general-purpose register holds the
   value it held before the call.
3. **Given** a build for a processor family with no marker instruction,
   **When** a program calls `simulation_start`, **Then** the call
   compiles, links, and returns, and no marker instruction appears in
   the emitted code.

---

### User Story 2 - Measure elapsed time immune to clock frequency adjustment (Priority: P2)

A performance engineer measures short-running benchmarks and stores the
results in a database for later comparison against other machines and
other releases. The wall-clock adjustments an operating system applies
to keep time synchronized cause the measured clock to run at a slightly
different rate than the hardware counter underneath it. Over a long
campaign of stored results, that rate difference accumulates into drift
between recorded durations and the durations a wall clock reports.

The engineer selects a nanosecond-unit clock counter whose rate is fixed
to the hardware counter, reads it before and after the measured region,
and stores the difference.

**Why this priority**: The library already publishes a nanosecond-unit
`machine/monotonic` counter that resolves to a platform clock served
without a system call. This story adds the rate-fidelity property the
existing counter lacks. It ranks below the trace marker because it
refines an existing capability. It opens no new one.

**Independent Test**: Sample the counter in a loop, assert consecutive
samples never decrease, assert agreement with the existing nanosecond
counter to within a stated rate tolerance, and record the per-read cost
distribution on the reference platform.

**Acceptance Scenarios**:

1. **Given** the machine root object of the counter catalog, **When** a
   caller resolves `machine/monotonic_raw` by its canonical address,
   **Then** resolution succeeds and the entry reports the `nanoseconds`
   unit.
2. **Given** a sampling loop reading the counter, **When** any two
   consecutive samples are compared, **Then** the later sample is
   greater than or equal to the earlier one.
3. **Given** a sampling loop reading the counter, **When** the smallest
   non-zero difference between consecutive samples is measured, **Then**
   no difference is finer than the resolution the platform reports for
   that clock.
4. **Given** sampling loops spread over several threads, **When** the
   threads are joined and their samples are compared in completion
   order, **Then** no sample is lower than one whose completion
   happens-before its own start.

---

### User Story 3 - Keep the emitted instruction faithful on every build (Priority: P2)

A maintainer changes the marker implementation, or a compiler upgrade
changes how it assembles. The marker stops matching what Intel SDE
recognizes, and every trace produced afterward starts at the wrong
place. Nothing in the test suite fails, because the marker has no
observable effect at runtime.

A maintainer wants the ordinary test run to fail the moment the emitted
instruction sequence changes, on both supported compilers, with no
requirement that Intel SDE be installed on the build machine.

**Why this priority**: The marker is the one feature in this repository
whose correctness is invisible to every existing gate. It ranks with the
counter because a silent regression here produces wrong traces, which
the project treats as worse than no output.

**Independent Test**: Deliberately alter the emitted sequence in a
scratch copy, run the gate, and confirm it fails; restore the sequence
and confirm it passes.

**Acceptance Scenarios**:

1. **Given** a supported compiler and a release configuration, **When**
   the marker translation unit is compiled and disassembled, **Then**
   the marker byte sequence appears exactly once inside the marker
   function.
2. **Given** a deliberately corrupted copy of the marker function,
   **When** the same gate runs, **Then** the gate fails and names the
   violation.
3. **Given** a build whose contract enforcement is fully enabled,
   **When** the same gate runs, **Then** the marker byte sequence still
   appears exactly once.

---

### User Story 4 - Keep the new surface inside the existing vocabulary (Priority: P3)

A maintainer reviewing the change needs the new counter to read like the
existing counters and the new API to read like the rest of the public
surface. Reviewers learn no second set of conventions.

**Why this priority**: This story is a property of the change. No user
waits for it. It ranks last because every other story's acceptance
depends on it holding.

**Independent Test**: Run the existing header-vocabulary scan and the
existing documentation-to-enforcement pairing gate; both pass with the
new files added.

**Acceptance Scenarios**:

1. **Given** the new public header and the new source file, **When** the
   header-vocabulary scan runs over the public headers, **Then** the
   scan finds no platform-counter or clock-syscall token in the new
   public header.
2. **Given** the new public declaration, **When** the
   documentation-to-enforcement pairing gate runs, **Then** every
   documented contract on the new declaration has a registered
   enforcement counterpart.
3. **Given** the build configuration, **When** a consumer compiles
   against the installed package, **Then** the new header is present in
   the install tree and reachable through the project's documented
   include path.

---

### Edge Cases

- **No tracer attached**: The marker is architecturally a no-operation.
  It executes as an ordinary register move followed by a one-byte
  no-operation, with no fault, no trap, and no side effect on any
  architectural state. Intel's own decoder classifies the second
  instruction as the base no-operation form and assigns it no marker
  attribute.
- **Register preservation across the call**: The marker moves a 32-bit
  immediate into the 32-bit view of a callee-saved register, which
  zeroes the upper 32 bits. The implementation preserves the full 64-bit
  register across the sequence, so a caller holding a pointer-shaped or
  tagged value in that register observes it unchanged. A test exercises
  the case where the upper 32 bits are set, because an implementation
  that preserves only the lower half still passes a test using small
  integers.
- **Repeated calls**: Each call emits one marker. An attached tracer
  counts each occurrence separately, which is what allows repeated
  regions.
- **Processor family with no marker instruction**: The call compiles to
  an empty function body. The tag stays published so callers can pass it
  to a tracer, and the documentation records that no marker is emitted.
- **Tag value at the width boundary**: The marker carries a 32-bit
  immediate. A value outside the unsigned 32-bit range fails the build
  through a compile-time assertion. Truncation stays silent in no
  configuration.
- **Assembler syntax**: The marker body is written in the assembler
  syntax the project's compiler drivers select by default. A build that
  switches the assembler to Intel syntax fails to assemble the marker
  body. The project selects no such switch today.
- **Marker placed inside a timed region**: The marker occupies two
  instructions and perturbs the measurement it sits inside.
  Documentation places it at the region boundary, outside the timed
  window.
- **Read-mode label contradicts the read path**: The new counter reports
  the read mode `syscall` while its read is served without a system
  call. A reader inspecting the catalog sees a label that names the
  slower path. The label stays for consistency with every existing clock
  counter, and FR-010 records the fast path in the published overhead
  table.
- **Two counters over one hardware counter**: The existing monotonic
  counter and the new nanosecond-rate counter derive from a single
  hardware counter. Their difference grows at the rate the operating
  system adjusts time against the hardware, measured at single-digit
  parts per million on the reference platform.
- **Existing per-platform branches**: The project carries preprocessor
  branches for operating systems it no longer supports, preserved for a
  future port. This feature leaves every one of them byte-identical.
- **Counter name collision**: The catalog refuses a duplicate canonical
  address. A counter registered at `machine/monotonic_raw` while an
  entry already holds that address fails registration and leaves the
  catalog unchanged, which the existing registration contract already
  covers.

## Requirements *(mandatory)*

### Measurement Core

- **FR-001**: The library SHALL publish a nanosecond-unit counter on the
  machine root object of the counter catalog under the canonical address
  `machine/monotonic_raw`, alongside the existing machine-root clock
  counters.
- **FR-002**: The new counter SHALL report the unit token `nanoseconds`
  and the read mode `syscall`, the read mode every existing clock
  counter reports, adding no new enumerator to either closed vocabulary.
  The label reads `syscall` while the read avoids the system call, and
  FR-010 records that caveat in the published documentation.
- **FR-003**: When a caller samples the new counter, the library SHALL
  read the platform clock through the path that avoids a system call,
  and SHALL return a cumulative unsigned 64-bit count of nanoseconds
  since the platform clock's epoch.
- **FR-004**: The library SHALL convert the platform's
  seconds-and-nanoseconds pair into a nanosecond count using integer
  arithmetic on a 64-bit type, with no floating-point step anywhere in
  the conversion.
- **FR-005**: The new counter SHALL be published on every supported
  build, because the platform serves the chosen clock through its fast
  path on every supported target.
- **FR-006**: The library SHALL document, as a postcondition of every
  sample, that the returned value is greater than or equal to the value
  returned by the immediately preceding sample of the same counter on
  the same thread.
- **FR-007**: The library SHALL document, as a postcondition of every
  sample, that the returned value is greater than or equal to the value
  returned by any earlier sample of the same counter whose completion
  happens-before the present sample's start. The platform documents no
  order between two samples taken on two threads with no ordering
  relation between them, so the library claims none.
- **FR-008**: The new counter SHALL be measurable without a reference
  oracle: a test SHALL establish that consecutive samples never
  decrease, that no observed step is finer than the resolution the
  platform reports for that clock, and that the per-read cost
  distribution matches the published figure for the reference
  platform.
- **FR-009**: All platform vocabulary naming the clock source, its
  identifier, and its reading function SHALL appear in the library's
  source files only, and SHALL NOT appear in any public header.
- **FR-010**: The library SHALL record the new counter's per-read cost
  in the published counter-overhead documentation, in the same table and
  column format as the existing clock counters, and SHALL record in that
  table's read-path column that the read is served by the platform's
  fast path, with no system call involved.

### Simulation-Start Marker

- **FR-011**: The library SHALL expose a public API named
  `simulation_start`, callable with no arguments, returning nothing, and
  declared `noexcept`.
- **FR-012**: When `simulation_start` executes on a build targeting the
  marker instruction set, it SHALL emit the two-instruction marker
  sequence: a move of a 32-bit immediate into the 32-bit view of the
  designated callee-saved register, followed by a one-byte no-operation
  carrying the segment-override prefix and the address-size-override
  prefix.
- **FR-013**: The immediate operand SHALL be the compile-time constant
  `0xFACE`, exposed through a named public constant that a caller passes
  to the tracer on its command line.
- **FR-014**: The library SHALL publish the tag constant's value in the
  public header's documentation, together with the tracer command-line
  option that consumes it and the byte order the tracer expects.
- **FR-015**: `simulation_start` SHALL preserve the full 64-bit
  designated callee-saved register across the emitted sequence, so that
  every general-purpose register holds the value it held before the
  call.
- **FR-016**: The library SHALL assert at compile time that the tag
  constant lies within the unsigned 32-bit range the marker's immediate
  operand provides.
- **FR-017**: `simulation_start` SHALL place no memory-ordering
  constraint on the caller, and the implementation SHALL record why no
  such constraint is required.
- **FR-018**: When `simulation_start` executes on a build targeting a
  processor family with no marker instruction, the call SHALL compile,
  link, and return, emitting no instruction sequence.
- **FR-019**: The library SHALL document, in the public header, that the
  emitted marker has no architectural effect, that a call is safe with
  no tracer attached, and that the call perturbs a measurement it is
  placed inside.
- **FR-020**: The library SHALL NOT ship the marker behind a
  preprocessor macro that the caller must invoke, and SHALL NOT accept a
  tag value at runtime.
- **FR-021**: The static-analysis suppression required for the marker
  statement SHALL carry its written justification in the comment on the
  same line, naming the check being suppressed and the reason the
  statement is required.
- **FR-022**: The marker's emitted statement SHALL NOT declare a memory
  clobber, and the implementation SHALL record the reasoning that an
  attached tracer observes executed instruction order directly.

### Verification

- **FR-023**: The library SHALL ship an automated gate, registered with
  the test runner, that compiles the marker translation unit at a
  release optimization level under each supported compiler and under
  each contract-enforcement setting, disassembles the result, and
  asserts that the marker byte sequence appears exactly once inside the
  marker function.
- **FR-024**: The gate specified in FR-023 SHALL plant a deliberately
  incorrect marker sequence in a scratch copy and SHALL fail when its
  detector fails to notice, so that a passing run cannot originate from
  a detector that inspects nothing.
- **FR-025**: The gate specified in FR-023 SHALL report a skip and exit
  successfully on a build whose compiler targets no marker instruction
  set.
- **FR-026**: The library SHALL ship a runtime test that loads a known
  value whose upper 32 bits are set into the designated callee-saved
  register, calls `simulation_start`, and asserts the register is
  bit-identical afterwards.
- **FR-027**: The library SHALL ship a runtime test that calls
  `simulation_start` repeatedly and asserts execution continues and
  terminates normally, covering the no-tracer-attached case.
- **FR-028**: The marker translation unit SHALL link with no tracer
  library, and no tracer tool SHALL appear among the project's
  dependencies.
- **FR-029**: The library SHALL extend the existing public-header
  vocabulary scan to cover the new public header, so that the scan's
  verdict on the new header is recorded on every run.
- **FR-030**: The new public declaration SHALL carry doxygen
  preconditions, postconditions, and invariants, and every documented
  contract SHALL have a registered enforcement counterpart in the
  documentation-to-enforcement pairing gate.
- **FR-031**: Where a documented contract of this feature has no
  runtime-observable effect to assert against, the library SHALL satisfy
  the pairing gate with a compile-time assertion and SHALL record in the
  plan which gate proves each remaining property.

### Repository Constraints

- **FR-032**: This feature SHALL NOT add a runtime dependency, a build
  option, or a preset, and SHALL NOT require any external tool to build,
  test, or install.
- **FR-033**: This feature SHALL NOT modify any preprocessor branch the
  project preserves for a future port, and SHALL NOT change any existing
  build configuration file.
- **FR-034**: This feature SHALL NOT add a provider, a registration
  macro, a dispatch table, or a counter base class. The new counter
  SHALL be a leaf entry in the existing clock provider's machine root,
  and the new API SHALL be a free function.
- **FR-035**: Every changed line SHALL trace to a requirement in this
  specification, and the change SHALL add no refactoring, comment
  rewording, or formatting outside the lines it must touch.
- **FR-036**: The library SHALL reach 100 percent line coverage, 100
  percent branch coverage, and 100 percent contract coverage on the code
  this feature adds, measured by the project's existing coverage gates.

### Key Entities

- **Nanosecond-rate counter**: The catalog leaf at
  `machine/monotonic_raw` on the machine root object, reporting the
  existing `nanoseconds` unit, sampled as a cumulative unsigned 64-bit
  nanosecond count, resolved at runtime by canonical string address
  through the existing catalog lookup.
- **Simulation-start tag**: A named public constant holding the value
  `0xFACE`, the unsigned 32-bit number a tracer is told to watch for.
  The value is part of the public contract because a caller passes it to
  the tracer on the command line.
- **Marker sequence**: The two instructions an attached tracer
  pattern-matches to decide collection has begun. It carries no
  architectural effect and leaves no state visible to the program after
  the call returns.
- **Codegen gate**: An automated test that disassembles the compiled
  marker translation unit and asserts the marker sequence's presence,
  count, and placement, standing in for the runtime observation the
  marker cannot provide.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A user places one call at a region boundary and, running
  the binary under Intel SDE with the tag documented in the public
  header, obtains a trace whose first collected instruction is inside
  the marked region.
- **SC-002**: With no tracer attached, a call to the marker leaves all
  16 general-purpose registers bit-identical to their pre-call values,
  including the designated callee-saved register loaded with a value
  whose upper 32 bits are set.
- **SC-003**: Every test run on a supported platform verifies the
  emitted marker byte sequence under both supported compilers and under
  both contract-enforcement settings, and a deliberate corruption of the
  sequence fails the run.
- **SC-004**: The nanosecond-rate counter's cost per read stays at or
  below 20 ns at the 50th percentile and at or below 25 ns at the 99th
  percentile over 10 million consecutive reads on the reference platform
  (AMD Ryzen 9 9950X3D, Linux 6.19, glibc 2.44). One read is bracketed by
  two timestamp-counter reads, the bracketing overhead is measured under
  the identical bracketing, the thresholds judge the overhead-corrected
  figure, and the uncorrected figure is published beside it. The existing
  per-sampling-action figures cover a wider quantity that includes the
  library's own machinery, so they set no threshold here.
- **SC-005**: Consecutive samples of the nanosecond-rate counter never
  decrease across 10 million consecutive reads on the reference
  platform, and across a further 10 million reads spread over every
  logical processor of that platform, where the cross-processor
  comparison holds between samples ordered by thread join.
- **SC-006**: The nanosecond-rate counter's measured rate agrees with
  the existing monotonic counter's measured rate to within 20 parts per
  million over a measurement interval of at least 60 seconds on the
  reference platform. The measurement runs as a local confirmation
  recorded in the feature's quickstart, and the ordinary test suite
  asserts the properties observable in milliseconds.
- **SC-007**: The new code reaches 100 percent line, branch, and
  contract coverage under the project's existing coverage gates, with
  the build asserting that untestable branches carry exclusion markers
  in the source.
- **SC-008**: The existing public-header vocabulary scan, the existing
  documentation-to-enforcement pairing gate, the formatting gate, and
  the prose-lint gate all pass with the new files present and
  unmodified.
- **SC-009**: The library installs and links into an external consumer
  project with the new header reachable, and the consumer's build links
  no tracer library.

## Assumptions

- The existing nanosecond-unit machine-root counter resolves to a
  platform clock served without a system call, and the project publishes
  its per-read cost. A new counter over that same clock source would
  carry the same cost and no new capability, which is why this
  specification proposes a counter over the hardware-rate clock instead.
- All platform clocks reporting nanosecond resolution share one fast
  path and one reading cost on the reference platform, measured within
  noise of each other. The choice among them rests on rate behavior
  under operating-system time adjustment, and a benchmark interval wants
  the clock whose rate the operating system leaves alone.
- The platform's fast path serves the chosen clock on every supported
  build. The operating system reserves a small set of clock identifiers
  for its own use, and the chosen identifier is outside that set on
  every supported target.
- The platform's own documentation states that this clock is guaranteed
  non-decreasing across threads. The kernel additionally clamps a
  detected backwards hardware-counter jump, so the guarantee holds even
  where the underlying counter is unsynchronized across processors.
- The project supports Linux on x86-64 alone, per its constitution. The
  non-marker path exists so that a future port compiles and runs. The
  coverage gate's exclusion markers cover it, because no test can
  exercise it on any supported build.
- The compiler drivers the project selects emit assembler syntax
  matching the marker body's syntax by default, and no build option
  changes that selection.
- The marker statement trips one active static-analysis check, which the
  project suppresses at other sites with a same-line written
  justification.
- The tag value is `0xFACE`, the value Intel's own marker wrapper uses
  for a start marker, so a reader familiar with that material recognizes
  it. No stop tag ships with this feature.
- A caller supplies the tag to the tracer on the tracer's command line.
  The library neither launches a tracer nor reads tracer output.
- The nanosecond-rate counter answers a rate-fidelity question, and a
  user needing only nanosecond resolution already has the existing
  counter. Scope covers the rate-fidelity counter alone.
- A region-end marker is out of scope. A caller who wants collection to
  end at a specific instruction passes both tags to the tracer's
  region-control option and places the closing marker in a later change.
- Multiple distinct regions in one process are out of scope, for the
  same reason.
- Making the existing monotonic counter cheaper by calling the
  platform's published time symbol directly is out of scope. Measured
  headroom over the current path is one to two nanoseconds, and the
  platform's own guidance on that interface advises against direct use.
- Version lineage: this feature adds public API without removing or
  changing any, so it ships as the next patch-level release under the
  precedent set by `specs/008-timestamp-counter`.

## Out of Scope

- A region-end marker, a stop marker, or any second tag.
- More than one start marker, and any nesting or region-identification
  scheme.
- Launching, configuring, or detecting Intel SDE or any other emulator.
- Reading or reporting a tracer's output.
- A runtime-configurable tag.
- Direct calls to the platform's published time symbols.
- Any change to the existing monotonic counter, to the raw hardware
  counter, or to any other existing catalog entry.
- Any change to the counter registration, selection, or read-dispatch
  machinery.
- Support for processor families beyond the marker instruction set,
  beyond the empty-path behavior described in FR-018.
- macOS and Windows support, which a future specification adds.

## Dependencies

- Intel SDE, an optional external tool, needed only to confirm SC-001
  locally. Its absence from a build machine affects no requirement,
  test, or gate in this specification.
