# Contract: Provider, Raw Time-Stamp Publication

**Feature**: `008-timestamp-counter` | Namespace `sg::counters` (R-001) | Entities: [data-model.md](../data-model.md)

This file is a delta. It records what 008 changes in the `clock` provider's
obligation over the time-stamp entry, the catalog facts the entry then
carries, and the deletions the removal of the calibration forces across the
provider and its shipped tests. Every other clause of
[007 provider-contract.md](../../007-counters-and-timers/contracts/provider-contract.md)
stands as merged and is not restated here: the registration base, the point
yield, the read modes, the availability reporting, the fast-mode protocol, and
the `push`, `fake`, and `linux_pmu` providers all stand. Where the two files
describe the same surface, this file governs the time-stamp entry.

Line numbers in the deletion tables name the merged 007 state, read from
`git show HEAD:` at the time of writing. They are anchors for review, and a
reviewer who has already applied the change reads the region names instead.

## The changed entry

007 declared a `tsc` entry under the `machine` object and gated it on a
sysfs frequency. 008 keeps the entry, keeps every field, and stops gating it
on anything at runtime.

| Field | Value after 008 | Change |
| --- | --- | --- |
| `name` | `"tsc"` | unchanged |
| `description` | a raw count; no rate is asserted | rewritten |
| `unit` | `unit::none`, which maps to `dim<0, 1>` at `include/speedgun-ng/counters_core.hpp:178` | unchanged |
| `avail` | `availability::countable` | unchanged |
| `mode` | `read_mode::fast_tsc` | unchanged |
| `frequency_hz` | `0`, the zero default, never written | stops being written |
| `scaled` | `false`, the zero default, never written | stops being written |

- **FR-001**: publication follows the build guard and nothing else. The
  guard is decided at compile time, so the entry carries no runtime state and
  the provider reads no file to decide it.
- **FR-002**: the count carries no rate. The frequency field keeps its zero
  default and the description asserts none, so no folded value pairs the count
  with a duration.
- **FR-003**: the reader opens exactly where the catalog enumerates, so one
  condition decides both sites.
- **FR-011**: this supersedes the calibration and publication obligations that
  FR-034 of 007 attached to this one entry. The rest of FR-034 of 007, and
  every other clock obligation, stand.

The provider gains no type, no enumerator, and no interface method. `tsc` was
already a declared entry name (FR-034 of 007) and `fast_tsc` was already a
mandated read-mode name (FR-023 of 007), so no public token is added and
`test/counters_header_purity.sh` needs no new exemption.

### Clause cells that change in the 007 table

The "Built-in providers shipped through this one contract" table of the 007
contract lists the `clock` provider. Two cells of that row change:

| 007 cell | 008 text |
| --- | --- |
| Leaves | `tsc`, x86 where the build executes the instruction |
| Notes | `tsc` asserts no rate; its frequency field and scaled flag keep their zero defaults (FR-002) |

The calibration-provenance sentence in that row goes, because the library no
longer computes the value the sentence described.

## The calibration the removal deletes

The read has always been a single instruction returning a count, at
`source/counters/clock_provider.cpp:146`. Everything the provider wrapped
around it treated the count as something needing a rate. Eight regions come
out. Each is named at its verified location in the merged 007 state.

### In the provider header

| # | Location | What goes |
| --- | --- | --- |
| 1 | `include/speedgun-ng/counters_clock.hpp:78` to `:85` | the private `tsc_calibration` struct with its three fields, the trailing `private:` label at `:77`, and the sole member `m_tsc`. Nothing reads either field once the seed and the reader key on the build guard. |

The class keeps its public surface. The constructor's `@brief` loses its
calibrating clause, and its `\pre none` and `\post none` lines stand.

### In the provider source

| # | Location | What goes |
| --- | --- | --- |
| 2 | `source/counters/clock_provider.cpp:206` to `:236` | the whole guarded constructor body, holding the sysfs open, the read of `/sys/devices/system/cpu/tsc_khz`, the parse, the nominal-frequency read from CPUID leaf 0x16, and the scaled comparison. The constructor becomes defaulted. |
| 3 | `source/counters/clock_provider.cpp:265` to `:288` | the catalog seed's runtime presence check at `:268` and its whole guarded body. The seed moves under the build guard, and the description changes from a calibrated rate to a raw count. |
| 4 | `source/counters/clock_provider.cpp:309` | the reader's presence condition. What is left is the unknown-address check, so the reader and the catalog agree on one condition. |
| 5 | `source/counters/clock_provider.cpp:139` to `:145` | the `LCOV_EXCL` region that spanned the whole time-stamp leaf. It existed because the calibration was host data no test could write. The exclusion narrows to the one arm a build without the instruction never reaches, and the two marker pairs wrapping the window's time-stamp arm go with it. |

Two supporting edits follow from the deletions above, both mechanical. The
`<fstream>` and `<cpuid.h>` includes leave
`source/counters/clock_provider.cpp` with the second `<cpuid.h>` guarded by
`__linux__`, and the translation unit's own header comment stops naming the
calibrated counter.

The read itself is untouched. `tsc_ticks()` keeps its P2 intrinsic
justification comment, which stands on the same reasoning after the change.

### In the shipped clock tests

| # | Location | What goes |
| --- | --- | --- |
| 6 | `test/source/counters_clock_push_test.cpp:218` to `:261`, with its scenario comment at `:210` to `:217` | the calibration scenario, deleted outright. Every assertion in it describes removed behaviour: the sysfs frequency read, the frequency provenance comparison, the scaled comparison against the CPUID nominal, and the description text check. |
| 7 | `test/source/counters_clock_push_test.cpp:286` | the entry-appearance assertion. Its condition inverts: the entry appears exactly where the build executes the instruction, so the assertion becomes the proof that publication follows the build guard. |
| 8 | `test/source/counters_clock_push_test.cpp:347` | the reader-equivalence assertion survives. Its stated reason is reworded from the calibration to the build guard, and it stays the check that proves item 4. |

Item 6 leaves three helpers with no caller, so they go as well. Each is dead
the moment items 6 and 7 land, and leaving them would be dead code:

| Location | What goes |
| --- | --- |
| `test/source/counters_clock_push_test.cpp:91` to `:94` | `contains`, called only by the description check in the deleted scenario |
| `test/source/counters_clock_push_test.cpp:96` to `:105` | `read_file`, the sysfs scalar reader, called only from the deleted scenario and from the inverted assertion |
| `test/source/counters_clock_push_test.cpp:107` to `:121` | `nominal_core_khz`, the CPUID leaf 0x16 reader, called only from the deleted scenario |

Their supporting scaffolding goes with them: the `SG_TEST_HAS_CPUID` guard at
`:34` to `:39`, the `<cpuid.h>` include it wraps, and the `<fstream>` include
at `:24` that `read_file` needs.

## What the provider still owes

Nothing else moves. The point yield, the one cumulative `std::uint64_t` per
managed leaf per sampling action, the unit and metadata as catalog facts, the
`noexcept` zero-allocation sample path, and the modular wrap handling all
stand as the 007 contract states them. The time-stamp leaf keeps its place in
the window's switch and keeps yielding through the same `point_sink`.

The count is cumulative and wraps like any other 64-bit cumulative counter in
the library. This feature asserts no epoch and no monotonicity across a
reset, which the 007 modular-delta rule already covers.

## Conformance list for the delta

| Clause | Guarantee | Requirement |
| --- | --- | --- |
| C-PRO-7 | The entry publishes wherever the build executes the instruction, with no runtime state and no file read | FR-001 |
| C-PRO-8 | The entry carries a count and no rate; the frequency field and scaled flag keep their zero defaults | FR-002 |
| C-PRO-9 | The reader opens the entry exactly where the catalog enumerates it | FR-003 |
| C-PRO-10 | No provider type, enumerator, or public token is added | FR-010 |
| C-PRO-11 | FR-034 of 007 stands for every clock leaf other than this one | FR-011 |
