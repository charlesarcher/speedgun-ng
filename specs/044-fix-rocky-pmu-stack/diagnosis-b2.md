# B-2 Diagnosis Report (harness_jitter_window_test)

**Date**: 2026-11-01. Host: CachyOS Linux, AMD Ryzen Threadripper 9960X
(24 cores, 48 hardware threads), kernel 7.3.0-5-cachyos. Preset
`ci-rocky`, base d454bcff. All runs local; no GitHub runner used.

## Reproduction counts (protocol section 2)

| Setup | Runs | Failures | Failure mode |
|---|---|---|---|
| `taskset -c 0,1` | 500 | 0 | none |
| `taskset -c 0` | 500 | 500 | Every run aborts at the test's own affinity precondition: `pickCpus` asks for two cpus, the mask holds one, so the failure is `the mask holds the cpus the scenario asks for (specs/027 FR-040)`; the refusal is environmental, and this setup yields no signal about the gate. The test cannot run on one cpu. |
| all cpus, one busy loop per cpu | 500 | 264 | 261 × `the second pass's window kept the jump its body scripted (specs/027 FR-022)`; 3 × `the document of a probed run loads (specs/027 FR-029, SC-001)` |
| Rocky 10 container (`docker.io/rockylinux/rockylinux:10`, the CI job's package list, no added load) | 500 | 0 | none |


The failure reproduces locally at a high rate under full-cpu load, so
the stop condition of the brief (2000 runs with no failure) does not
apply: the diagnosis went forward.

## B-1 pre-fix evidence

The seam test on the master build aborted 1 of 200 plain runs
(`/tmp/b2-diag/fail-seam-prefix-*.out`), which sits below the brief's about 3 of 100 (the brief measured 6
of 200) and confirms the stack-smashing abort reproduces on this
host. Inside the Rocky 10 container the master build's seam
test failed 3 of 200 runs: 2 aborts with the stack-smashing message
and 1 segmentation fault with no output, which the fix's 1,000-run
validation covers.

## Instrumented captures (temporary debug prints, never committed)

A local build printed, at each gate wait and at the failing checks:
the `readActions()` values, the spins, and the kept-gap stats; and
per-thread read attribution inside the fake provider's `readPoints`.

Per-thread attribution at `bmPassGate`'s second wait unblock (two
failing runs):

- One thread's count grows by millions each pass: the probe cpu's
  rides (the scenario probes one cpu).
- One thread's count is small and grows by a few thousand: the
  runner's own sampling reads.
- One thread's count freezes at exactly 1,249,970 in both runs: an
  earlier scenario's reader thread whose reads ended deterministically.

The failing runs' document stats: `gapCount=8388608 minNs=1000000
maxNs=1000000`: the fold's maximum kept gap stays at the first
pass's jump (1,000,000) while the second pass's window must carry
2,000,000, though the probe rode the jumped tail millions of times
between the two waits.

## Root cause (high confidence)

`bmGate` and `bmPassGate` wait on `provider.readActions()`, one
process-wide atomic that every fake-window read on any thread
increments: the probe cpus' rides, the runner's sampling reads, and
every other window's reads. The waits' margins (20,000 actions;
two times the first call's action count plus 64) are calibrated
against assumptions about those shares that do not hold under load.
When the margin unblocks on reads that are not the probe's rides of
the pass's jump, the pass's timed window closes with no kept gap at
the pass's magnitude, and the fold carries only the earlier
magnitude; that unblocking is the failure the CI run reported at the
gate entry check (specs/027 FR-027, E-08) and the local runs report
at the second pass's check (specs/027 FR-022).

## Fix direction

The fake provider's seam already stands where the fix goes: the
reader-install seam (`installTestReader`, `source/harness/detail/
jitter_probe.hpp`) stands a test reader in place of the one the
probe opens over the `machine/tsc` leaf, and the fake provider's
public `open()` hands the scripted window over the same leaf. The
test installs a counting reader that delegates to the provider's
window and counts the probe's own reads, kept apart from the
runner's reads; the gate waits end on that count, so a wait ends on
an event the test controls and not on a large margin. The waits
keep their spin bound, so a stalled probe still reports a failure.

## Second root cause (found during the fix's validation)

The first validation of the wait fix still failed the loaded runs at
two checks of the roundtrip scenario: the gate entry's fold check and
the document load. Instrumented captures tied both to one cause: the
fake's `setPoints` resets the shared hand-off slot (`item.last = 0`)
on every call, and the gate bodies reprogram the series mid-run. The
runner's window-edge stamps fork from that slot, so every mid-run
reprogram collapsed or wrapped the edges: the window's `[start, end)`
placed none of the probe's stamps, the entry's fold carried the
earlier magnitude (the gate-entry failure the CI job reports), the
fold's covered span underflowed or stood at zero, and the writer
emitted `gapsPerSecond` at the double's ceiling, which the loader's
number rule refused.

Two defects carried the fix:

- `setPoints` keeps the shared cursor when the reprogram carries no
  points (a tail-only reprogram continues the series instead of
  restarting it), so the stamps another reader forks from the slot
  stand ordered across the reprogram.
- The fake window's hand-off leaves the shared cursor alone when the
  window's own cursor stands below it, so the two probe cpus' cursor
  drift cannot drag the slot backward.
- The counting reader holds one delegate window per probe thread,
  the shape the probe's default path gives every item, so a cpu's
  gap stays the scripted tail and never the count of another cpu's
  reads in between.
