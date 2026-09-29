# Implementation Plan: Raw Time-Stamp Counter

**Branch**: `008-timestamp-counter` | **Date**: 2026-09-29 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/008-timestamp-counter/spec.md`, grounded in the frozen record of `specs/007-counters-and-timers/` and in the verified reading of the shipped source.

**Artifacts**: [research.md](research.md) · [data-model.md](data-model.md) · [quickstart.md](quickstart.md) · [contracts/system-contract.md](contracts/system-contract.md) · [contracts/provider-contract.md](contracts/provider-contract.md)

## Summary

008 makes the time-stamp counter entry raw. The read at `source/counters/clock_provider.cpp:146` is a single instruction returning a count, and everything 007 wrapped around it treats the count as something that needs a rate. The constructor at lines 206 to 236 reads `/sys/devices/system/cpu/tsc_khz` and CPUID leaf 0x16, and line 268 withholds the entry unless that read succeeds. So the entry is attached to a calibration, and it disappears on a host whose counter works perfectly and publishes no frequency.

Removing the calibration makes the feature small. Publication follows the build guard, which is already decided at compile time, so the presence flag, the file read, the instruction-identifier read, and the scaled computation all go. What remains is one catalog seed, one reader condition, one accessor, and the repairs that removal forces in the shipped tests.

The counter stays a counter. It keeps `unit::none`, which maps to `dimension {.time = 0, .events = 1}` at `include/speedgun-ng/counters_core.hpp:178`, so `instructions / tsc` folds to a `dim<0,0>` ratio exactly as `instructions / cycles` does on a counted source. No new type, no new enumerator, no new algebra, and no value that pairs a count with a duration.

## Technical Context

**Language/Version**: C++23 (`CMAKE_CXX_STANDARD=23`, `cxx_std_23` PUBLIC, `CMAKE_CXX_EXTENSIONS=OFF`). CMake >= 3.20. No new dependency.

**Primary Dependencies**: zero. No library, no vendored tree, no tool. The feature removes two file and instruction reads and adds nothing.

**Storage**: N/A. The entry carries no captured state once the calibration goes.

**Testing**: the repository's frameworkless convention. Hand-rolled `check()`/`fail()` executables in `test/source/`, plain `add_test` in `test/CMakeLists.txt`. A dependency-scan test forbids adding a test framework, so none is added.

**Target Platform**: Linux (GCC/Clang) is the full feature, since the instruction is x86. Elsewhere the build compiles, the entry is absent, and the accessor reports it. That is the reduced-catalog behaviour 007 already models (FR-042 of 007, zero API difference). No macOS and no Windows CI job exists; constitution 2.7.0 suspends Windows MSVC and 2.8.0 defers macOS to developer-local, and the corrected SC-006 names only the Linux matrix.

**Project Type**: C++ library feature delivered through the existing single library target, plus one new test executable, one repaired test, one benchmark edit, and one documentation page.

**Performance Goals**: the read is the cheapest count the library can return, and SC-004 requires a documented budget confirmed in the release preset. Measured on the development host, back-to-back reads cost a 1-tick minimum with a 42-tick median, against 86 ticks for the monotonic clock and roughly 194 ticks for a full plan sampling action. The distribution is bimodal, because a read pair can overlap inside the out-of-order window, so the minimum is the floor and the median is an upper bound. Publishing the entry changes no read cost. It removes the condition under which the read was unreachable, and it removes a file read from every clock-provider construction.

**Constraints**: the entry carries a count and no rate (FR-002), and its frequency field keeps the zero default, so no folded value implies a duration the count does not carry; publication is decided at build time with no runtime state and no file read (FR-001); the reader opens exactly where the catalog enumerates (FR-003); the accessor performs no hardware read, constructs no plan, and mints no recorder (FR-005); the accessor does not open the registration boundary (FR-006); the contract pairing gate is bidirectional, so each documented contract line and its enforcement site agree, and enforcement sits lexically inside the method body; the header purity gate bans `rdtsc` and the standalone acronym `PMU` from the public headers, so public prose says "time-stamp counter" and "instruction", and the FR-008 message naming the absent counter lives in the source only.

**Scale/Scope**: one catalog seed moved under the build guard, one reader condition moved, one calibration struct and two reads deleted, one accessor declared and defined, one test executable added, one existing test repaired by deletion, one benchmark edited, one documentation page edited. No new type, no new enumerator, no new source file, no new header, no CMake change, and no change to any existing signature.

## Constitution Check

*GATE: must pass before implementation. Re-checked after design: still passing.*

| Principle | Status | Notes |
| --- | --- | --- |
| I. Standard-First Coding | PASS | C++23, extensions off, no new dependency, and one dependency-shaped thing removed: a hand-rolled sysfs parse and a hand-rolled instruction-identifier read, both of which existed only to decorate a count. The one remaining P2 exception is inherited and unchanged: the time-stamp intrinsic at `source/counters/clock_provider.cpp:149`, whose recorded justification stands. |
| II. Design By Contract | PASS | The accessor's doxygen `\pre` and `\post` sit on their own lines, paired with runtime enforcement per the gate's bidirectional rule. The FR-008 absent-counter check is a recoverable error and never a contract violation, so it is a plain `if` and not an `SG_REQUIRE`. No new contract macro is introduced anywhere. |
| III. R-DCUT | PASS | spec, then this plan, then tasks, then code and tests. TDD recorded in the Test Plan. |
| IV. Documentation | PASS | The accessor carries full doxygen. The entry's description states that it is a raw count asserting no rate, which is the disclosure a caller reads. The budget and its measured figure are published in the existing overhead page. |
| V. Style and Formatting | PASS | `.clang-format` governs. No new file, so `cmake/lint.cmake` needs no change. |
| VI. Test-Backed Code | PASS | A new test executable covers every new requirement, and an existing test loses the assertions that described the removed calibration. Coverage is 100% line, 100% branch, and 100% DBC, and deleting the calibration removes the host-dependent branch that previously needed a coverage marker. |
| VII. Performance Discipline | PASS | The read is a designated critical path with a budget measured in the release preset and published. The bimodal distribution is reported as a minimum and a median. Construction loses a file read. That is a side benefit, recorded as such and not claimed as a goal. |
| VIII. CI Quality Gates | PASS: nothing weakened | No gate configuration changes. Two test targets touched. The `prose-lint` job needs at least one commit on the branch because it exits 2 on an empty range. clang-tidy and cppcheck report without failing the build, so the release build log is read and compared against the pre-change baseline. |
| IX. Spec-Driven Development | PASS | All four artifacts under `specs/008-timestamp-counter/`. Public API work, so the bypass does not apply. The mandatory release-preset build runs before the tasks are called done. |
| X. Anti-Slop | PASS | One catalog seed, one reader condition, one accessor, zero new types. An earlier draft proposed a reading value carrying its own unit and calibration, a span type, a difference operator, a nanosecond conversion, and a defaulted virtual on the provider; all five are withdrawn, and the withdrawal is recorded in the spec and in research.md. The calibration struct and its two reads are deleted, never left dormant, which is the deletion this row asks for elsewhere. |
| XI. Discourse and Prose | PASS | Generated prose in this directory follows XI and is verified with the prose gate in whole-tree mode, since the range mode reads only committed content. |

**Gate-set note (Principle VIII)**: nothing weakens. No signature changes, no vtable changes, no build-configuration change. The behavioural correction lands in a merged feature, and the successor log records it as a correction so no silent edit ships.

## Project Structure

### Documentation (this feature)

```
specs/008-timestamp-counter/
├── spec.md                      the feature specification
├── plan.md                      this artifact
├── research.md                  the raw-counter decision and the withdrawn design
├── data-model.md                what changes and what does not
├── quickstart.md                runnable validation scenarios
├── contracts/
│   ├── system-contract.md       the accessor delta
│   └── provider-contract.md     the raw-publication delta
└── checklists/
    └── requirements.md          requirements-quality review record
```

### Source Code (repository root)

```
include/speedgun-ng/
└── counters_system.hpp          + system::tsc declaration

source/counters/
├── clock_provider.cpp           the seed moves under the build guard, the
│                                reader condition moves, the calibration
│                                struct and both reads are deleted
└── system.cpp                   + system::tsc definition, a tree lookup

test/
├── CMakeLists.txt               + one four-line registration
└── source/
    ├── counters_tsc_test.cpp    new executable
    ├── counters_clock_push_test.cpp   the calibration scenario is deleted
    └── counters_overhead.cpp    brackets a sampling action with the counter

docs/pages/
└── counters-overhead.md         + the budget and its measured figure
```

## Design: Logical View

### Public surface added

```text
  namespace sg::counters

  counters_system.hpp
    class system
      expected<counter<dim<0,1>>, error> tsc() const

  machine catalog, after 008
    name          "tsc"
    description   a raw count; no rate is asserted
    unit          unit::none            -> dim<0,1>, unchanged from 007
    avail         availability::countable
    mode          read_mode::fast_tsc   -> the single-instruction read
    frequency_hz  0                     -> the zero default, never written
    scaled        false                 -> the zero default, never written
```

No type is added, no enumerator is added, and the entry's field list is unchanged. Two fields stop being written, because nothing in the library computes them for this entry any more. `dim<0,1>` is the dimension `unit::none` already maps to, so the composed expression type-checks through the existing algebra with no special case.

### Sequence: one ambient lookup

```text
  caller              system::tsc()          the tree
    |                      |                     |
    |-- tsc() ------------>|                     |
    |                      |                     |
    |                      | does NOT call       |
    |                      | ensure_open(), so   |
    |                      | registration stays  |
    |                      | open (FR-006)       |
    |                      |                     |
    |                      |--- look up "machine"|
    |                      |<-- the machine node-|
    |                      |                     |
    |                      |--- look up "tsc" --->|
    |                      |<-- the leaf record--|
    |                      |                     |
    |                      | build counter from  |
    |                      | the leaf record     |
    |<-- expected<counter---|                     |
    |    <dim<0,1>>,error>-|                     |
```

No hardware read happens. No plan is compiled. No recorder is minted. The accessor is the same shape as `object::counter<D>(name)` and returns the same type, so the two are interchangeable at the call site.

### Cross-cutting correctness promises

- **Publication follows the build guard.** FR-001, FR-003. One condition decides both the catalog seed and the reader, so a published entry always opens.
- **A count carries no rate.** FR-002. The frequency field keeps its zero default, the description asserts no rate, and no folded value pairs the count with a duration.
- **The boundary is never opened.** FR-006. The accessor is a pure lookup, so a program may call it before or between registrations.
- **The counter is interchangeable.** FR-004, FR-008. One type, one unit, one algebra, one fold path, shared with every counted source.
- **No epoch is claimed.** The count is cumulative and wraps. This feature asserts no monotonicity across a reset.

## Design: Physical View

### Files and their duties

| File | Change | Duty |
| --- | --- | --- |
| `source/counters/clock_provider.cpp` | edit | seed the entry under the build guard, align the reader, delete the calibration struct and both reads |
| `include/speedgun-ng/counters_system.hpp` | edit | the accessor declaration and its contract |
| `source/counters/system.cpp` | edit | the accessor definition, a tree lookup |
| `test/source/counters_clock_push_test.cpp` | edit | delete the calibration scenario, reword the reader assertion |
| `test/CMakeLists.txt` | edit | one four-line registration |
| `test/source/counters_tsc_test.cpp` | new | the coverage |
| `test/source/counters_overhead.cpp` | edit | the budget measurement |
| `docs/pages/counters-overhead.md` | edit | the published budget |

### What deletion forces in 007

Checked against the shipped sources, with each item read out of the file:

1. `source/counters/clock_provider.cpp:78` to :85 declares the private calibration struct and its sole member. Once the seed and the reader both key on the build guard, nothing reads either field, so the struct and the member go.
2. The constructor body at lines 206 to 236 exists only to fill that struct. With the struct gone the whole guarded block goes, and with it the sysfs read, the parse, the nominal-frequency read, and the scaled comparison.
3. The catalog seed at lines 265 to 288 moves from a runtime `if` to the build guard, and its description changes from a calibrated rate to a raw count.
4. The reader at line 309 drops the presence condition, because the build guard now decides.
5. The `LCOV_EXCL` region opening at lines 139 to 145 exists because the calibration is host data no test can write. With no calibration the region closes, and the coverage marker it carried goes with it.
6. `test/source/counters_clock_push_test.cpp:218` to :232 is a scenario asserting the frequency, its provenance, and the nominal comparison. Every assertion in it describes removed behaviour, so the scenario goes.
7. `test/source/counters_clock_push_test.cpp:286` asserts the entry appears exactly when the platform published a frequency. Its condition inverts, and the assertion becomes the proof that publication follows the build guard.
8. `test/source/counters_clock_push_test.cpp:347` asserts the reader opens exactly where the catalog publishes. It survives as the check that proves item 4, with its stated reason changed to the build guard.

### Build targets and link relationships

No build-configuration change, and none is needed. `CMakeLists.txt:658` globs `source/counters/*.cpp` with `GLOB_RECURSE` and `CONFIGURE_DEPENDS`, and `cmake/install-rules.cmake:15` installs the whole `include/` directory, so neither a new translation unit nor a new header would require an edit, and 008 adds neither. The test registration is four lines in `test/CMakeLists.txt` inside the 007 counters section. The export macro is class-level, so the new member carries none.

## Test Plan

### Execution mode: TDD (recorded per Principle III)

The test executable is written first and verified failing, then the implementation turns it green. Each increment lands as its own commit, so the branch head is green, and the commit body names the `git bisect` consequence so nobody meets a red commit by surprise.

### Coverage strategy

Coverage is 100% line, 100% branch, and 100% DBC. The feature makes coverage simpler than it was.

- **Publication is compile-time.** The seed and the reader sit under the build guard, so there is no runtime branch and no coverage marker for one. The test guards its own assertions on the same condition, following the `SG_TEST_HAS_*` pattern at `test/source/counters_clock_push_test.cpp:32`.
- **The removed calibration leaves no gap.** Deleting the constructor block and the presence branch removes the lines and the branch that previously required the marker at `source/counters/clock_provider.cpp:139`. The new test asserts the seed's fields directly, so the fields it no longer writes are still covered.
- **The unregistered-provider path** needs a fresh process, because `system::local()` is a singleton, so the first statement of `main` calls `tsc()` before any registration. Reordering silently loses the branch.
- **Interchangeability** is asserted by composition, with type identity left implicit: the new test builds an expression from the accessor's result and a counted source, compiles it, and folds it.

### Scenario and check mapping

| Scenario | Requirement | Check |
| --- | --- | --- |
| The catalog lists the entry | FR-001 | the `machine` catalog carries a `tsc` entry at `read_mode::fast_tsc` |
| Publication ignores any frequency | FR-001 | the entry is listed on this host, which publishes none |
| The entry asserts no rate | FR-002 | the frequency field is 0, the scaled flag is false, and the description states a raw count |
| The reader opens the entry | FR-003 | the existing reader assertion, reworded to the build guard |
| The accessor returns the same counter as the named lookup | FR-004 | both name `machine/tsc` |
| The accessor reads nothing | FR-005 | the catalog is identical before and after a thousand calls |
| The accessor leaves registration open | FR-006 | a provider registers successfully after a call |
| No provider registered | FR-007 | a recoverable error naming the absent provider |
| A build without the instruction | FR-008 | compile-time, guarded in the test the same way |
| The counter composes in an expression | FR-002 | a quotient against a counted source compiles and folds to a `dim<0,0>` ratio |
| The budget is published | SC-004 | the overhead page carries the release-preset figure |

### Gate sequence

The order avoids wasted work, because a pairing or coverage failure means a code change means a re-run.

1. `cmake --preset=dev && cmake --build --preset=dev && ctest --preset=dev` after each increment. The pairing gate is the highest-risk gate for a new public function, so it runs early: `cmake --build build/dev -t dbc-gate`, which needs `doxygen` on `PATH`.
2. `python3 tools/prose/prose_gate.py --check all --mode tree`. The `--mode tree` flag is mandatory; the default range mode reads only committed content and reports a false green.
3. `cmake --preset=ci-ubuntu && cmake --build build`, the mandatory Principle IX release build, run alone so the budget measurement does not contend with other work. The log's `warning:` count is compared against the pre-change baseline, because clang-tidy and cppcheck report without failing the build.
4. `cmake --preset=ci-sanitize && cmake --build build/sanitize`, then ctest with the CI option block, `strict_string_checks=1`, `detect_stack_use_after_return=1`, `check_initialization_order=1`, `strict_init_order=1`, `detect_leaks=1`, `halt_on_error=1`, and `UBSAN_OPTIONS` with `print_stacktrace=1` and `halt_on_error=1`.
5. `test/counters_header_purity.sh` and `test/counters_push_atomic_scan.sh`.
6. Local coverage is delegated to the CI coverage job, because genhtml's perl `GD` module is absent on the development host. The reason is recorded so the omission reads as a decision.
7. `cmake -P cmake/prose-lint.cmake` before the push, plus the commit check.

## Complexity Tracking

| Trade | Cost | Ceiling | Upgrade path |
| --- | --- | --- | --- |
| The entry publishes with a zero frequency field | A caller reading that field finds 0 and learns no rate is attached. | One field, carrying the zero default every unset counter already carries. | None needed. A caller wanting a rate supplies it. |
| The accessor duplicates the named lookup | Two spellings reach one entry. | One member, one delegation, same return type. | Remove the accessor if the short spelling proves unused. |
| The correction withdraws a merged requirement's calibration | 007's text promises a calibrated entry and 008 ships a raw one. | One requirement, recorded as superseded in the spec and the successor log. | None. The successor log is 007's own mechanism for a correction. |
| The reader condition and the seed condition must agree | Two sites encode one condition. | Two sites, both under the build guard. | None. A shared predicate would be a second thing to keep in step. |
| No test measures the read on a machine that publishes a frequency | Nothing verifies a rate the library no longer computes. | Nothing, because the library computes no rate. | None needed while the entry stays raw. |

**Version decision**: 008 is additive at version 0.1.0. It adds one member to `system`, moves one publication condition to the build guard, stops writing two fields on one entry, and changes no existing signature and no vtable. `SOVERSION` stays at 0 and the change ships as 0.2.0. The constitution requires a deliberate version decision on a public API change, and this records it.
