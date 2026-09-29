# Data Model: Raw Time-Stamp Counter

**Feature**: `008-timestamp-counter` | **Date**: 2026-09-29 | **Plan**: [plan.md](plan.md)

Entities per spec "Key Entities", with fields, validation rules, and state transitions. Requirement ids cite [spec.md](spec.md); mechanism ids cite [research.md](research.md); an id followed by 007 cites [the 007 spec](../007-counters-and-timers/spec.md). Line references cite the tree as 007 left it, which is the state the change starts from.

No entity is added and no field changes. The feature publishes an entry 007 already declares, stops writing two of its fields, adds one accessor returning the counter type 007 already defines, and deletes a private struct together with the block that filled it. Recording that precisely is the whole content of this artifact.

## E-01 The catalog entry `machine/tsc`

The entry, field by field, before and after. The field list is unchanged in both columns (E-03 of 007).

| Field | Type / shape | Before | After |
|---|---|---|---|
| `name` | string, unique within the owning object | `"tsc"` | `"tsc"`, unchanged (FR-034 of 007) |
| `description` | string | names the sysfs frequency in MHz and states the comparison against the nominal core frequency | asserts a raw count and no rate (FR-002, User Story 1 scenario 3) |
| `unit` | unit token plus its dimension mapping | `"none"` | `"none"`, which `include/speedgun-ng/counters_core.hpp:178` maps to `dimension {.time = 0, .events = 1}` |
| `avail` | `countable` / `permission_blocked` / `not_encodable` / `absent` | `countable` | `countable`, unchanged |
| `mode` | achieved read mode, disclosed per entry | `read_mode::fast_tsc` | `read_mode::fast_tsc`, unchanged (FR-023 of 007) |
| `frequency_hz` | frequency in hertz | the sysfs `tsc_khz` value converted to hertz | 0, the zero default, never written (FR-002) |
| `scaled` | `bool` | the comparison against CPUID leaf 0x16 | false, the zero default, never written (FR-011) |

Validation: the entry is listed exactly where the build executes the instruction, and that condition is the build guard (FR-001). The reader at `source/counters/clock_provider.cpp:309` drops its presence condition, so the catalog and the reader key on one condition and a listed entry always opens (FR-003). Nothing validates two of these fields at runtime any more, because nothing writes them.

State transitions:

```text
listed (the build executes the instruction) --> listed, counting raw ticks
unlisted (the build does not)                 --> absent from the catalog
```

## E-02 Fields that stop being written

| Field | Type / shape | Notes |
|---|---|---|
| `frequency_hz` | `std::uint64_t` | nothing in the library computes a frequency for this entry any more, so the field keeps the zero default it was born with (FR-002, FR-011) |
| `scaled` | `bool` | the comparison that set it read the instruction-identifier leaf the deletion removed, so the field keeps the zero default (FR-011) |

A zero frequency and a false scaled flag are the state every unset counter already carries, so no reader of the catalog meets a shape it has not met before. A caller wanting a rate supplies it from outside the library, which is what the spec's assumptions record. No folded value pairs this count with a duration, so nothing downstream implies an elapsed time the count does not carry (SC-003).

## E-03 The accessor

One member added to `system`, and no other public type.

| Field / operation | Signature shape | Notes |
|---|---|---|
| `tsc` | `[[nodiscard]] auto tsc() const -> std::expected<counter<dim<0, 1>>, error>`, declared in `include/speedgun-ng/counters_system.hpp` and defined in `source/counters/system.cpp` | FR-004; the body resolves the `machine` node and its `tsc` leaf and builds the counter from that leaf record, which is the record `object::counter<D>()` builds from at `include/speedgun-ng/counters_system.hpp:116` |

Validation: the return type is the type the uniform lookup returns, so the two spellings are interchangeable at the call site and a caller learns no new type (FR-004, SC-002). The accessor performs no hardware read, constructs no plan, and mints no recorder (FR-005). It does not call `ensure_open()`, so a program may call it and then register a provider (FR-006). It is `const`, and a `const` member cannot open the boundary, which is the same constraint stated once (R-004).

State transitions: none. The call reads the tree and returns a value, so it holds nothing and leaves nothing behind.

## E-04 Errors returned by the accessor

Both failures are recoverable `std::unexpected` values carrying the `error` of E-06 of 007, shaped the way `source/counters/system.cpp:277` to :365 shapes them: a lowercase message, the failing condition named, a rationale clause, the requirement id in parentheses.

| Condition | Returned | Notes |
|---|---|---|
| no clock provider registered | `std::unexpected`, no counter | returned before any read; the program may still register one afterwards (FR-007, User Story 2 scenario 2) |
| the build publishes no `tsc` entry | `std::unexpected`, no counter | the guard is private to `source/counters/clock_provider.cpp`, so the tree answers this case and no read happens (FR-008, User Story 2 scenario 3) |

Validation: neither condition is a contract violation and neither carries a contract macro, because a host without the instruction and a program that registered nothing both have a legal answer to give (R-006). The accessor documents `\pre none` and `\post none`, matching `object::counter<D>()` at `include/speedgun-ng/counters_system.hpp:110` to :113, so the `dbc-gate` pairing has no documented line left unpaired (FR-009, R-009).

## E-05 State that disappears

The calibration and everything that filled it. Stated as transitions, with the before and the after.

| Item | Before | After |
|---|---|---|
| `clock_provider::tsc_calibration`, the private struct | `include/speedgun-ng/counters_clock.hpp:78` to :85, holding `present`, `khz`, and `scaled` | deleted; no reader remains for any of the three fields |
| `clock_provider::m_tsc`, the member | one instance of that struct | deleted |
| the constructor block | `source/counters/clock_provider.cpp:206` to :236, opening `/sys/devices/system/cpu/tsc_khz`, parsing it, reading CPUID leaf 0x16, and comparing the rate | deleted whole; the constructor retains nothing |
| the presence flag | `m_tsc.present`, set at line 224 and read at lines 268 and 309 | gone; the build guard decides at both sites |
| the frequency comparison | `nominal_khz != khz` at line 232 | deleted |
| the coverage exclusion | the region opening at `source/counters/clock_provider.cpp:139` to :145, which the file's comment says ends with the constructor | closed with the constructor at line 236; its markers go |
| the seed's coverage markers | the region at lines 265 to :288 and the branch marker at line 268 | gone; the seed is unconditional and covered |
| the reader's presence arm | `index == kTscIndex && !m_tsc.present` at line 309 | gone; the reader keys on the build guard |
| the calibration scenario | `test/source/counters_clock_push_test.cpp:218` to :232, asserting the frequency, its provenance, and the nominal comparison | deleted; every assertion in it describes removed behaviour |
| the publication assertion | `test/source/counters_clock_push_test.cpp:286`, asserting the entry appears exactly when the platform published a frequency | its condition inverts to the build guard |
| the reader assertion | `test/source/counters_clock_push_test.cpp:347`, asserting the reader opens exactly where the catalog publishes | survives, with its stated reason reworded to the build guard (FR-003) |
| the build-guard pattern for the new test | `test/source/counters_clock_push_test.cpp:32` to :39, the `SG_TEST_HAS_*` convention | reused as it stands, so the new test guards its assertions the way the existing suite does |

The provider's construction transition, in the shape the other state tables use:

```text
before: constructing --(the sysfs read succeeds)--> calibrated {present, khz, scaled}
        constructing --(the sysfs read fails)-----> uncalibrated, entry withheld
after:  constructing ----------------------------> default-constructed, nothing read, nothing stored
```

The new test follows the build-guard convention at `test/source/counters_clock_push_test.cpp:32` to :39:

```text
SG_TEST_HAS_x86 == 1 --> the assertions run: the entry is listed, raw, countable
SG_TEST_HAS_x86 == 0 --> the assertions skip, with the reason named
```

## E-06 Cross-entity invariants

1. The catalog and the reader agree on one condition: both key on the build guard, so a listed entry always opens and an absent entry is never requested (FR-001, FR-003).
2. The entry's unit keeps the composed expression inside the existing algebra: `unit::none` maps to `dim<0,1>`, so a quotient against a counted source folds to a `dim<0,0>` ratio with no new enumerator and no special case (FR-002, R-005).
3. No folded value pairs this count with a duration: `frequency_hz` stays 0 and `scaled` stays false, so a fold over this entry reports a count and the disclosure every other fold reports, with no rate attached (FR-002, FR-011, SC-003).
4. The accessor returns what the named lookup returns: both build a counter from the same leaf record, so the two spellings name the same canonical entry and are interchangeable at the call site (FR-004).
5. The accessor reads nothing and opens nothing: it holds no state, touches no recorder column, and leaves the registration boundary closed, so a program may call it before, between, or after registrations (FR-005, FR-006).
6. The feature adds no vocabulary: `tsc` and `fast_tsc` are tokens 007 already required, and the accessor carries the same token, so the header purity gate's vocabulary list does not grow (FR-010, R-007).
