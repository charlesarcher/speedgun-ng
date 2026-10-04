# Contract: `machine/monotonic_raw`

**Feature**: `specs/011-nanosecond-counter-ssc-mark/spec.md`
**Date**: 2026-10-03

The public surface of the nanosecond-rate counter: how it is published, what
it guarantees, how it is read, and the shape of the row it adds to the
published overhead table.

## Catalog entry

```text
address      machine/monotonic_raw
name         monotonic_raw
unit         nanoseconds
read mode    syscall
availability countable
dimension    time
```

The address is composed by the catalog as the object's path plus `/` plus
the leaf name. It is derived, never supplied, so a duplicate address is a
duplicate name and the existing registration check covers it.

## Resolution

Two steps, the same two every counter takes:

```cpp
namespace sg::counters {
// Every consumer declares this alias itself, as the examples and the tests do.
using time_dim = dim<1, 0>;

const auto machine = system::local().object("machine");
const auto raw = machine.counter<time_dim>("monotonic_raw");
}  // namespace sg::counters
```

No single-call resolver for a full leaf address exists in the public surface
and this counter adds none. The resolved handle is interchangeable with
`machine/monotonic`: same dimension, same unit, same handle type.

## Read path

| Property | Value |
|----------|-------|
| Clock | `CLOCK_MONOTONIC_RAW`, served by the kernel from the hardware clocksource's own rate |
| Call | libc `clock_gettime` through `<ctime>`, which glibc resolves to the vDSO symbol `__vdso_clock_gettime64` at start-up |
| Conversion | `tv_sec * 1000000000ULL + tv_nsec`, integer only, on an unsigned 64-bit type |
| Result | cumulative nanoseconds since the platform clock's epoch, unsigned 64-bit |
| System calls | none on a build whose clocksource the vDSO can read; the call degrades to a real system call when the `vdso=0` boot parameter is set or the clocksource's `vdso_clock_mode` is `VDSO_CLOCKMODE_NONE` |
| Direct symbol calls | none; the platform's published time symbols stay out of this feature |

## Guarantees, and their conditions

1. A sample is greater than or equal to the immediately preceding sample of
   the same counter on the same thread. The platform states this for every
   `CLOCK_MONOTONIC` variant and permits equal values.
2. A sample is greater than or equal to an earlier sample on another thread
   when that earlier sample's completion happens-before this sample's start.
   The unconditional cross-thread form is not a platform guarantee: the
   kernel's clamp against a backwards hardware-counter jump holds the value
   for the reader that observed it and says nothing about another processor.
   Cross-processor comparability on x86 rests on the synchronized timestamp
   counter, and a kernel that distrusts its counter switches clocksource.
3. The smallest non-zero step between samples stays within the resolution the
   platform reports for this clock, which `clock_getres` answers from the
   kernel-wide timer mode flag. That flag is `1` nanosecond once the kernel
   has switched to high-resolution timers. It is a kernel-wide value, and no
   clocksource property enters it.

The specification's FR-007 states the second guarantee with this condition,
and the clarification session of 2026-10-03 records why. research.md R-005
carries the evidence for the kernel's per-reader clamp.

## Rate property

The clock's rate is the hardware clocksource's rate, unslewed. NTP slew and
step adjust `CLOCK_MONOTONIC` and `CLOCK_REALTIME` and never reach this
clock, which is the property that distinguishes it from `machine/monotonic`.
The price is that its rate carries the hardware's own frequency error and
drifts against UTC without correction, so it answers a rate-fidelity question
and a caller needing only nanosecond resolution already has the existing
counter.

## Read-mode label

The catalog reports the read mode `syscall`, the label every clock counter in
this library carries, while the read is served without a system call. The
enumeration is closed and FR-002 forbids a new enumerator, so the label stays
and the fast path is recorded in the published table's read-path column, where
a reader of the counter catalog will look for it.

## Published overhead row

One row per build-configuration table in `docs/pages/counters-overhead.md`,
in the existing column format:

```text
| plan | min ns | median ns | max ns | read mode |
```

The new row names the counter, carries the measured figures from the
overhead harness, and writes the read-path cell as the fast path with no
system call involved. The page states that it is not a CI gate, so the row is
checked against the harness's printed run. No automated comparison exists.

Measured figures and the measurement convention are recorded in
research.md R-006 and in the plan's Technical Context.

## Rejected alternatives

| Alternative | Why rejected |
|-------------|--------------|
| A vDSO read-mode enumerator | FR-002 forbids a new enumerator in a closed vocabulary that three other providers read |
| A direct call to the platform's published time symbol | Outside this feature's scope, with one to two nanoseconds of headroom over the current path |
| `CLOCK_BOOTTIME` | Suspend-inclusive counting would invalidate the rate comparison against `machine/monotonic` |
| A floating-point conversion | FR-004 requires integer arithmetic, and a floating-point step has no place on this path |
| A new provider or a registration macro for this leaf | FR-034 forbids both, and the existing clock provider's machine root is where the leaf belongs |