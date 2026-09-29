# Research: Raw Time-Stamp Counter

**Feature**: `008-timestamp-counter` | **Date**: 2026-09-29 | **Plan**: [plan.md](plan.md)

Requirement ids cite [spec.md](spec.md); an id followed by 007 cites [the 007 spec](../007-counters-and-timers/spec.md). Entity ids cite [data-model.md](data-model.md). Line references cite the tree as 007 left it, which is the state the change starts from.

The feature was designed three times and the owner corrected it twice. The corrections are the most informative part of this record, because each rejected design names the defect the shipped design then fixes. The correction record comes first, then the decisions the surviving design rests on.

## Correction record

### Design one, rejected

The first design proposed a reading value type carrying the raw count, the unit, the source, a calibrated flag, the frequency provenance, and a counter-invariance flag. Beside it it proposed a difference operator, a span type, a nanosecond conversion, and a defaulted virtual on `provider_iface` returning a clock calibration, so `system` could find the clock provider without a downcast.

The owner rejected it with:

> "tsc is just a counter, like everything else we count in rdpmc; it must
> have the same interface as all other counters to be used interchangeably."

The design failed on three counts, and each failure is a fact about the library as it already stands.

1. The catalog entry discloses the frequency and the scaled flag itself. FR-034 of 007 makes the calibration provenance catalog data, so a reading carrying the same two facts restated the catalog in a second place.
2. FR-019 of 007 already attaches the value, the running ratio, and the scaled flag to every fold. The reading type restated the library's own disclosure model, and two copies of a disclosure model drift apart.
3. A counter carrying a bespoke interface stops being interchangeable with the other counters, which was the requirement the owner stated. Interchangeability is the feature: the point is to drop this counter into an expression beside any other.

### Design two, rejected

The second design revised the first. The parallel types stayed, and a `system` accessor was added beside them.

The owner rejected it twice, first with

> "anything dealing with frequency is out of scope, we're just COUNTING in
> this PR, not associating time with the counter",

and then with

> "raw TSC, nothing to calibrate, the read is a counter".

Three consequences follow, and they are the shape of the shipped design.

- The calibration is withdrawn outright. FR-011 records the withdrawal as superseding the calibration and publication obligations of FR-034 of 007 for this counter.
- The invariance flag goes with it. Invariance is an interpretation of a count: it answers whether a difference spanning a thread migration is sound, which is a question about time. The owner's second correction put time out of scope, so the flag has no home.
- The span type, the difference operator, and the nanosecond conversion go too. Each existed only to turn a count into a duration, and the duration is what the owner removed.

What survives from both designs is one thing: a short spelling for the entry. The owner's phrase "the read is a counter" means the call returns the library's counter type, with no new type between the caller and the count.

## R-001 The entry keeps the counter interface

**Decision**: the time-stamp entry stays an ordinary counter. It is resolved by name through `object::counter<D>()`, it composes under the dimension algebra, and it folds with the disclosure FR-019 of 007 attaches. Nothing about it is reachable except through those three doors. The feature adds one spelling for the entry and removes the calibration that surrounded it (FR-002, FR-004).

**Rationale**: the read at `source/counters/clock_provider.cpp:146` is one instruction returning a count, and a count is what the library already knows how to carry end to end. Interchangeability is also what makes the counter useful in the library's own tooling, where it has to enter an expression beside a counted source with no new type to learn (User Story 3, SC-002). The parallel type family bought nothing the counter type lacks, and it cost the one property the owner asked for.

**Alternatives considered**: the reading, span, difference, and conversion types of design one (each restates the catalog's disclosure per FR-034 and FR-019 of 007, and each breaks interchangeability); a thin alias over `counter<dim<0,1>>` that carries a rate (the rate is the withdrawn calibration, and the alias would be the only counter in the library carrying one); returning the bare `uint64_t` (a bare number states neither its source nor its unit, so a caller cannot tell it from any other count, and it bypasses the closed unit mapping of FR-017 of 007).

## R-002 Publication follows the build guard

**Decision**: the catalog seed moves under the build guard, and the reader's presence condition goes with it, so one condition decides both (FR-001, FR-003). The presence flag, the sysfs read, the instruction-identifier read, and the scaled comparison are deleted. The entry's description changes from a calibrated rate to a raw count, and the two rate-bearing fields keep their zero defaults (FR-002).

**Rationale**: the guard is defined at `source/counters/clock_provider.cpp:23` under `#if (defined(__x86_64__) || defined(__i386__)) && !defined(_MSC_VER)`, and x86-64 mandates the instruction, so a build that enables the guard always executes the read. The condition at line 268 answered a different question, the one R-007 of 007 asked: whether a calibrated leaf could be published. With no rate on the count, nothing consumes that answer. The file it read, `/sys/devices/system/cpu/tsc_khz`, is kernel data, so every clock-provider construction paid a file read to fill three fields the library no longer computes.

**Alternatives considered**: keep the runtime probe and publish an uncalibrated entry when it fails (keeps a branch and a file read whose only product is a boolean nothing reads); probe the instruction at runtime through the instruction-identifier leaf (the architecture mandates the instruction, and the probe costs more than the read it guards); publish the entry unconditionally and report a host without the instruction as not-present at lookup time (the catalog would claim a counter on a platform that cannot read it, and FR-042 of 007 treats the entry's absence as a catalog fact); keep a second runtime check in the reader (two sites encoding one condition is how they drift).

## R-003 Reaching the provider: four routes, none needed

**Decision**: the accessor is a pure tree lookup carrying no platform knowledge. It resolves the `machine` node, finds the `tsc` leaf, and builds a counter from that leaf record, exactly as `object::counter<D>()` does at `include/speedgun-ng/counters_system.hpp:116`. It is defined in `source/counters/system.cpp`, the file that already holds `system::object()` at lines 367 to 383 and `system::register_provider()` at lines 277 to 365. It reaches no provider, and `provider_iface` gains no member, so the vtable is unchanged and there is no ABI consequence (FR-004).

**Rationale**: the routes to the provider's private state were each evaluated, and each breaks a rule the repository already holds. A friend declaration on `clock_provider` naming `system` grants friendship to read three scalars and still provides nowhere to call the read, because the read function at line 146 is private to the anonymous namespace spanning lines 33 to 161. A public method on `clock_provider` makes internal plumbing public API, because `include/speedgun-ng/counters.hpp:12` puts everything public in that header into the documented single include, which subjects it to the header purity gate and the contract pairing gate. A new internal header under `source/counters/detail/` carrying the read forces the instruction into a shared header, and the file's own header comment at lines 1 to 4 confines platform terms to this translation unit. Routing through the catalog leaf record's provider index, the way `source/counters/plan.cpp:477` to :509 does, cannot serve this lookup at all, because that record exists only once the entry has been enumerated, and the enumeration is the decision this accessor has to make. The route that breaks none was available from the first draft.

**Alternatives considered**: the four routes above; `dynamic_cast` on the stored base pointers (the tree contains none, and their absence settles the question); a non-virtual provider-kind tag (a tag carries no data, so a tag could identify the provider and nothing more); a separate registry of clock providers (a second registration path beside the caller-driven one, holding state that can disagree with the provider list).

## R-004 The accessor reads nothing and opens nothing

**Decision**: `tsc()` is `const`, performs no hardware read, constructs no plan, and mints no recorder (FR-005). It does not call `ensure_open()`, so a program may call it and then register a provider (FR-006).

**Rationale**: constness and the absence of `ensure_open()` are one constraint seen from two sides. Opening the boundary closes provider registration (FR-009 of 007), and the absent-provider error is defined in the pre-open state, so an accessor that opened the boundary could not observe the state its own error describes. A `const` member cannot open anything, so both halves fall out together.

A second fact settles the absent-counter path. The guard is defined inside `source/counters/clock_provider.cpp`, so `source/counters/system.cpp` cannot test it and does not need to. A build without the guard never seeds the leaf, so the lookup finds no `tsc` under `machine` and returns the recoverable error naming the absent counter, having read nothing (FR-008). The absent-counter answer is therefore a property of the tree, and the accessor needs no second platform test.

**Alternatives considered**: call `ensure_open()` and treat a pre-open system as ready (a read that transitions state on the caller's behalf, and it makes a later `register_provider` fail for a reason the caller cannot see); cache the resolved counter at the open boundary (the pre-open state is exactly the state the absent-provider error describes, and a provider registered afterwards would never be found); make the accessor non-const so that it may open (a mutating call whose every use is a read); publish the guard in a shared header so the accessor can test it (that is the shared-header route of R-003, and it moves a platform term out of the one unit that owns it).

## R-005 The unit stays `unit::none`

**Decision**: the entry keeps `unit::none`, which `include/speedgun-ng/counters_core.hpp:178` maps to `dimension {.time = 0, .events = 1}`. No enumerator is added, no algebra changes, and no composed expression needs a special case. A quotient against a `dim<0,1>` counted source folds to a `dim<0,0>` ratio, the shape `instructions / cycles` already produces on a counted source (FR-002).

**Rationale**: the dimension lives in the type, so the entry's dimension is a compile-time fact the type system carries, and 007 already assigned the token. A new enumerator would give one unit two spellings and would widen the closed vocabulary that FR-017 of 007 and the header purity gate both rely on. This is why the feature needs no new algebra: the count already sits where a count belongs.

**Alternatives considered**: a new `unit::ticks` enumerator (two tokens for one unit, and FR-010 adds no vocabulary); mapping the entry to `unit::nanoseconds` so a rate metric type-checks (the entry carries no rate, so the number would be a count wearing a time unit's name, and SC-003 forbids a caller reading the entry learning anything past the count); leaving the unit unset (FR-017 refuses an unrecognized token at registration, so the seed would never enter the tree).

## R-006 The two recoverable errors

**Decision**: two conditions return `std::unexpected` through a plain `if`, each carrying an `error` built in the shape `source/counters/system.cpp:277` to :365 already uses: a lowercase message, the failing condition quoted or named, a rationale clause, and the requirement id in parentheses. No contract macro appears on the accessor. No clock provider registered returns the error naming the absent provider and no counter (FR-007). No `tsc` entry under `machine` returns the error naming the absent counter and no counter, having read nothing (FR-008).

**Rationale**: both conditions are recoverable states a legal program may meet. A host without the instruction has a legal program to write, and a program that registered no provider has a legal recovery in registering one. Tier-3 contract machinery reports a violation and terminates, which is the wrong tier for either. The error type at `include/speedgun-ng/counters_core.hpp:106` holds a `std::string` message and a `std::vector<std::string>` of suggestions, so both failure paths allocate; that is the cost 007's tree lookups already pay, and it is why the accessor carries no `noexcept` even though it performs no read.

**Alternatives considered**: `SG_REQUIRE` on either condition (terminates a program the spec says receives a returned error); a configuration error at startup when no provider is registered (registration is the caller's choice and may follow the call); an empty `expected` with no message (the caller learns that something is missing and nothing about what); a hard failure on a host without the instruction (the spec requires a returned error, and the library still has to build there).

## R-007 Vocabulary and the header purity gate

**Decision**: the public core vocabulary gains no token. `tsc` is already required vocabulary, because FR-034 of 007 declares a `tsc` entry and FR-023 of 007 mandates `fast_tsc` as a read-mode name, and `test/counters_header_purity.sh` declines to scan `tsc` on that ground. The accessor carries the same token, and public prose says "time-stamp counter" and "instruction" while the platform spellings stay in the source (FR-010).

**Rationale**: the purity gate scans the public headers for the platform terms the read would otherwise drag in, and 007 already made both tokens required, so the gate already declines them. The FR-008 message names the absent counter by its catalog name and is built in the source, so the spelling never has to appear in a public comment.

**Alternatives considered**: rename the accessor to a phrase (a second vocabulary for one counter, and the owner asked for the token twice); document the read's cost by naming the instruction in the public header (the gate scans comments too, so the measured figure lives in the counters overhead page instead, per SC-004).

## R-008 The coverage exclusion closes

**Decision**: the `LCOV_EXCL` region opening at `source/counters/clock_provider.cpp:139` to :145 closes with the calibration, and its markers go. The seed's own region at lines 265 to :288 and the branch marker at line 268 go with the branch they marked, and the reader's presence arm at line 309 goes with the markers on that arm.

**Rationale**: the region existed because the calibration is host data no test can write, since `/sys/devices/system/cpu/tsc_khz` is a kernel file. With no calibration there is no host-dependent line left to exclude, and the seeded entry is now covered on every host the suite runs. A marker left behind after its reason is gone excludes live code, and a silent hole in the coverage denominator is worse than a recorded one.

**Alternatives considered**: keep the markers (they exclude covered code and hide real gaps); widen the region to cover the accessor as well (the accessor is in a different translation unit, so the region cannot reach it); move the measurement into a test-only provider (the point of the feature is the shipped provider's entry).

## R-009 Contract surface, pairing, and version

**Decision**: the accessor carries a doxygen contract of `\pre none` and `\post none`, the pair `object::counter<D>()` already carries at `include/speedgun-ng/counters_system.hpp:110` to :113. The prose describing which error each condition returns sits in the doc body above that pair, where the gate's parser does not read it (FR-009). The version stays 0.1.0, `SOVERSION` stays 0, and the change ships as 0.2.0.

**Rationale**: the pairing gate is bidirectional, so a documented contract line with no enforcement site is a finding in the same way that an enforcement site with no documented line is. A documented precondition the caller cannot promise, a registered provider among them, would force a check that terminates a program the spec says receives a returned error (FR-007). The vtable is unchanged, because `provider_iface` gains no member, so no symbol a consumer links against by name disappears or changes signature. The soname therefore stays at 0, and the minor version is what records the addition.

**Alternatives considered**: document "a clock provider is registered" as a precondition and pair it with `SG_REQUIRE` (terminates a legal program, and the spec asks for a returned error); document a positive postcondition and pair it with an `SG_INVARIANT` (the returned value is an `expected` whose failure is a documented outcome, so an invariant over it says nothing a caller can act on); bump `SOVERSION` to 1 (no removed or renamed symbol forces a relink).

## Open questions

None. Every Technical Context item in [plan.md](plan.md) resolves to an R-entry above. The two items left for implementation are the release-preset measurement that fills the published budget (SC-004) and the CI matrix run, and both carry out a decision an R-entry above already made.
