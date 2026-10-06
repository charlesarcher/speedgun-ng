# Contract: the gap state and the disclosure column

This contract covers F-01, F-02, and I-05. Every declaration below is a
public contract, and each is paired with the enforcement that guards it
(Principle II).

## What this feature changes

Three things, and nothing else:

1. A plan names a disclosure column per window. One column serves the whole
   plan no longer.
2. A fold result and a raw view carry an availability field.
3. A fold reads the disclosure mark at both of its end points.

`point_sink::put` keeps its one-integer signature
(`counters_provider.hpp:212`). Every change here is another column, never a
wider call.

## `point_sink::put`

**Signature, unchanged**: one `std::uint64_t` per leaf per sampling
action.

**Contract**: a window writes each managed leaf's count through this call,
in the order `leaf_set::addresses` gives, and writes the disclosure column
in the same pass.

**Enforcement**: `SG_REQUIRE(value != detail::no_disclosure_column)` on
every write that is not the disclosure column's own turn, so a window that
names no column writes no mark and a window that names one writes it
exactly once per action.

## `leaf_set::disclosure_column`

**Contract**: a window that owns at least one managed leaf receives a
disclosure column of its own. A window with no leaf of its own receives
`no_disclosure_column` and writes no mark.

**Why the shape changed**: at the audit point the compile names the
column to the one window that owns the plan's last managed leaf, so a plan
mixing PMU and clock leaves loses every PMU gap whenever the clock
provider registered second. Per-window naming removes the order
dependence: SC-002 drives a PMU gap under both registration orders and
reads the same result.

**Enforcement**: the compile asserts that every window receiving a leaf
received a column, and `SG_ENSURE` at the window's write site that the
column index is inside the window's own range.

## `struct metric_result`

**Location**: `include/speedgun-ng/counters_core.hpp:265-270`

| Field | Contract |
| --- | --- |
| `value` | the delta over the window, or no value when the state is `gap` |
| `running_ratio` | the fraction that ran, over `(0, 1]` |
| `scaled` | the figure was scaled to a declared unit |
| `availability` | **new**: the state the disclosure column holds |

**Contract on `availability`**: it holds the value the disclosure column
holds for the same sampling action, and `gap` when the start point carries
the mark too. It is never `countable` while `value` is absent.

**Why an availability and not a boolean**: the disclosure column already
carries an `availability`, and `counters_core.hpp:87-95` already
separates a permission refusal, an encoding refusal, an absent entry, a
scope refusal, and a gap. A fold that publishes the column's value names
the reason. A boolean would answer only that a gap exists, which is the
state F-02 names as the defect.

**Enforcement**: `SG_REQUIRE(availability != availability::gap ||
value_or(0.0) == 0.0)` at construction, so a gapped fold cannot carry a
value, and `SG_REQUIRE(availability != availability::countable ||
value.has_value())` in the other direction, so a caller never reads a
`countable` state beside no value.

## `struct points_view`

**Location**: `include/speedgun-ng/counters_measurement.hpp:391-401`

The same field, the same contract, the same enumeration. A caller reads a
raw view's state here and names no column index.

**Enforcement**: the accessor that builds a view asserts the state it
read is the column's own value, `SG_ENSURE(availability ==
static_cast<availability>(column[disclosure_column]))`.

## What this feature removes

The only reader of the disclosure column outside the library is
`counters_recorder_test.cpp:247`, which addresses it by the literal index
2. That test reads the field instead. After this feature no source outside
`fold.cpp` and the provider sources names a column index at all, which is
the state SC-003 measures.

## The fold's two reads

**Contract**: before computing a delta, a fold reads the disclosure column
at the start point and at the end point. A `gap` at either end suppresses
the value and publishes `gap`.

**Why two and not one**: the counts are cumulative, so a gap strictly
inside a window leaves both end points measured and the delta correct. One
read misses a gap that opens the window, which is why `fold.cpp:45`
subtracts a real count from zero and why a window whose start point
carries the mark reports its whole cumulative count as the delta.

**Enforcement**: `SG_REQUIRE(!(start == gap || end == gap) ||
!value.has_value())` on the fold's return, and a test that drives a gap
at the start point, at the end point, and strictly inside one window.

## The running ratio after the change

At the audit point `leaf_ratio` at `fold.cpp:116-126` reads the disclosure
at the end point alone, and `fold.cpp:150-154` and `fold.cpp:180-182` fall
back to 1.0. A gapped window therefore reports the running ratio of a
clean full-rate window, which is what F-02 records.

After this feature a fold that publishes `gap` reports no ratio at all,
because a ratio over a window with no measured delta describes nothing.
A fold whose window measured publishes its ratio unchanged. A multiplexed
window is unaffected, and its ratio still comes from the enabled and
running pair under D-04.

**Enforcement**: the fold's contract states that `running_ratio` is
meaningful only when `availability != gap`, and the test asserts a gapped
fold reports `gap` and no value.

## Sampling cost

None of this runs on the sampling path. The disclosure is written once per
action per window, which the clock window already does today. The two
extra reads happen in the fold, which runs once per figure. The overhead
page records the measured median, and FR-027 gates it at five percent of
the pre-fix median.
