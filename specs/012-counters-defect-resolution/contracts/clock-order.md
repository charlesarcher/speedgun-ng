# Contract: clock leaf order guarantees

Governs `include/speedgun-ng/counters_clock.hpp`. Requirements: FR-028,
FR-029, FR-030, FR-031.

## Class contract

`clock_provider` states no order on any leaf's behalf. Each leaf states
its own guarantee, and every guarantee a leaf states has a test that
exercises it (FR-028, FR-029).

**Doxygen** for the class: seeds the `machine` object with the
`monotonic`, `monotonic_raw`, `thread_cpu`, and `process_cpu` leaves in
nanoseconds at system open, plus the `tsc` leaf where the build
executes the instruction. No order is stated here. Each leaf's own
clause states its guarantee.

**Enforcement**: `SG_REQUIRE` that the provider registered no leaf
whose documented guarantee its clock does not provide. The check runs
once at registration, on the setup path, and never on a sampling path.

## `machine/monotonic`

`CLOCK_MONOTONIC` read through the vDSO.

**Guarantee**: a sample does not fall below an earlier sample taken on
the same thread, across the process's life.

**Enforcement**: `SG_ENSURE` that the calibration bracket reads a
non-negative span, which a falling clock would make negative.

**Test**: two samples on one thread, the second not below the first.

## `machine/monotonic_raw`

The same clock, unadjusted by `NTP`.

**Guarantee**: as `machine/monotonic`.

**Enforcement**: as `machine/monotonic`.

**Test**: the same test drives both leaves, so the pair is compared
under one protocol.

## `machine/thread_cpu`

Per-thread CPU time.

**Guarantee**: a sample is non-decreasing on the thread that took it.
A new thread's first sample can fall below an earlier sample taken on
another thread, because the counter is per thread (FR-030).

**Enforcement**: none. A cross-thread comparison is not a precondition
the library can check, because the library holds no shared state between
two threads' samples.

**Test**: a new thread's first sample falls below an earlier sample
taken on another thread, and the documented guarantee permits the
result.

## `machine/process_cpu`

Per-process CPU time.

**Guarantee**: a sample does not fall below an earlier sample taken on
another thread of the same process, because the counter is the
process's.

**Enforcement**: `SG_ENSURE` on the calibration bracket that the span is
non-negative, the same check `machine/monotonic` carries.

**Test**: a sample on a second thread does not fall below one taken on
the first.

## `machine/tsc`

The processor's cycle counter, read with `_rdtsc` and no ordering
fence.

**Guarantee**: the value is ordered only when one thread takes both
window endpoints. This precondition is on the caller (FR-031). The leaf
claims no order across threads and no order across processors. It gains
no ordering fence, so its per-read cost does not rise.

```
\pre One thread takes both endpoints of the window this leaf measures.
\post none
```

**Enforcement**: `SG_REQUIRE` that the build executes the instruction
where the entry is published. The cross-thread precondition has no
enforcement, because a thread that reads across threads cannot be
checked at a cost the hot path accepts. The library publishes the
precondition and states that a fence is absent.

**Test**: both endpoints are read on one thread, and the leaf is read
with no fence, asserted by the existing
`test/counters_tsc_read_shape.sh` gate.

## What this feature does not add

No fence reaches any leaf. A fence helps only a caller that reads across
threads, and the harness takes both window endpoints on one thread, so a
fence would cost a read per timestamp for no correctness the caller
lacks (FR-031, Principle VII).
