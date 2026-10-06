# Contract: fast-path reads, folds, and the disclosure column

Governs `include/speedgun-ng/counters_provider.hpp` and the read path in
`source/counters/linux_pmu/`. Requirements: FR-002 through FR-008,
FR-045, FR-046.

## `point_sink::put`

**Doxygen**: appends one cumulative point to the next managed column.

```
\pre Fewer than `leaf_count` points have been put this action.
\post The point lands in the column matching the call index, at the
      constructed row; the call index advances by one.
```

**Enforcement**: `SG_REQUIRE(index < m_leaf_count, ...)` before the
write and `SG_ENSURE` after it, both already present at
`include/speedgun-ng/counters_provider.hpp:204-210`.

**Signature**: `void put(const std::uint64_t value) noexcept`. It keeps
one integer and gains none (FR-007). The disclosure travels as one more
managed column the sampling action writes through this same call, beside
the ratio pair's two columns.

## `point_sink::check_action`

**Doxygen**: checks that a finished action wrote one point per managed
column.

```
\pre none
\post none
```

**Enforcement**: `SG_INVARIANT(m_index == m_leaf_count, ...)`, already
present. The read path calls it where the row is committed, so a read
that writes fewer columns than the plan compiled fails where the
shortfall is visible.

## The disclosure column

One managed column per sampling action. A measured action carries the
countability value the catalog publishes for the entry that column's
leaf resolved to. An action that measured nothing carries
`availability::gap`, which the catalog never publishes for an entry
because an entry spans many actions.

| Property | Value |
| --- | --- |
| Written by | the window's read thunk, through `point_sink::put` |
| Carries | `sg::counters::availability` for that action |
| Per-sample metadata | none is added (FR-007) |
| On a measured action | the entry's own countability value |
| On an action that measured nothing | `availability::gap` |

A caller that reads the column knows every count beside it was measured
under the state the column names. A caller that sees a zero count reads
the column to learn whether the zero is a measured zero or a gap. The
measured zero carries the entry's own countability value beside it, and
the gap carries `availability::gap` beside it. A sentinel count would
publish a wrong value with no disclosure, so no sentinel ships.

## Decode recipe

`fast_decode` at `source/counters/linux_pmu/fast_read.cpp:73-99`. The
recipe is named here by its content, because no kernel header path or
line number is part of this contract.

**Order**, as the kernel's own interface header documents it:
the capability gate, then the one-based index the instruction takes,
then the sequence comparison that closes the window, then the offset
and the counter width.

**Recipe**: sign extend the value the instruction read from the
published `pmc_width` bits, then add the page's `offset`. The point
carries no width mask, so a cumulative count past the published counter
width stays a cumulative count (FR-004).

**Enforcement**: `SG_ENSURE` that a returned `ok` verdict leaves `value`
equal to the offset plus the sign-extended instruction value.

## Verdicts and the fallback

| Verdict | Meaning | Point | Disclosure |
| --- | --- | --- | --- |
| `ok` | the page published a stable readable value | the decoded count | the entry's countability value |
| `not_allowed` | the page grants no capability, or indexes no counter | zero | `availability::gap` |
| `unstable` | the page sequence moved across the read | zero | `availability::gap` |

The retry the protocol states runs once. A retry that also fails takes
the same arm, and no value read before the sequence moved survives into
the point (FR-002, FR-003). The fallback issues no syscall read of the
same event, so the sampling action keeps the cost this library
publishes for it.

## Group read

A leader read that returns fewer bytes than the group header, and a
group read the syscall refuses outright, both write `availability::gap`
into the disclosure column and put zero into the member columns. No
fold the caller performs over that action reports a delta from a count
the read never produced (FR-006, FR-007).

**Enforcement**: `SG_REQUIRE` that `scratch` covers the header and one
word per member of the widest group, which the window constructor
establishes. No count is fabricated to fill a column.

## Enabled and running pair

The pair rides the leader's page in fast mode and the leader's group
read in syscall mode. The window publishes `time_enabled` and
`time_running` as the page holds them; the extrapolation is applied in
`leaf_ratio`, which divides the running delta by the enabled delta
(FR-005).

A pair read that discloses no stable value writes a zero pair and
`availability::gap` into the disclosure column. A caller reading the
disclosure column then knows the ratio it holds was not measured.

## Pure decisions on kernel-facing paths

Every corrected decision on a path the kernel alone can execute is
extracted into a small pure function declared in
`source/counters/detail/pmu.hpp`, the project's existing private seam
header. Each such function carries a doxygen clause and a paired
enforcement site, and a registered test drives it over synthetic
input. The corrected decision reaches the coverage gates through that
function.

The repository already holds two of these functions, and they are the
shape FR-046 names: `fast_index_valid`, which decides whether the fast
read is permitted from the one-based index, and `fast_pair_stable`,
which decides whether a seqlock read is stable. A registered test covers
both arms of each, over synthetic pages.

A coverage-exclusion marker standing on the kernel-facing wrapper that
a granted event alone can enter stays in place, because no single host
reaches that wrapper. The marker standing on a release arm of a
corrected decision goes, because a registered test now reaches it
(FR-027, FR-046).

## Sampling cost

`recorder::sample()` stays `noexcept`, allocation-free, and lock-free.
The disclosure column adds one managed-column write per sampling
action, and no branch reaches the action from a contract check (FR-008,
FR-045).

## Cpu-target pinning

**Doxygen**: a cpu-target fast-mode plan requires its sampling thread
to run on the processor the plan opened its contexts on.

```
\pre The calling thread runs on the processor this plan opened its
     contexts on.
\post none
```

**Enforcement**: `SG_REQUIRE` on the sampling path, semantic-gated. A
migrated thread is caught in every configuration that emits contract
code; a release build configured `ignore` emits no check, and the
per-sample cost stays where FR-008 holds it. The requirement adds no
branch to `recorder::sample()` (FR-045).
