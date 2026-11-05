# Contract: the provider fill, its order and its one probe

**Feature**: specs/041-counter-probe | **Files**: `source/counters/detail/limit_fill.hpp`
with `source/counters/linux_pmu/limit_fill.cpp`, seated at `source/counters/linux_pmu/provider.cpp`
| **Audit point**: `858c55d9`
**Spec**: FR-018, FR-020, FR-022, FR-030 to FR-034, FR-040a, FR-042, FR-043, D-1, D-4,
D-6, D-8, PC-3, PC-5

## What this contract covers

One pass fills one `ObjectSeed`. This contract fixes where the probe stands inside that
pass, what a device pays, which value loses a counter to the watchdog, and how the source
reaches the record beside the figure. The search itself stands in `group-probe.md`, the
device rule in `device-rule.md`, and the source surface in `limit-source.md`.

## The seat, and why no later seat works

The published limit travels in `ObjectSeed::programmableCounters`
(`include/speedgun-ng/counters_provider.hpp:78`), the provider fills it at
`provider.cpp:842-853`, and `System` stages each seed at registration
(`source/counters/system.cpp:305`), which PC-5 freezes. A probe that ran at the first
plan would answer after the stage, so the figure would reach no reader. FR-022 therefore
settles the probe inside the enumerate pass, once per device, at the first catalog open
of the process (D-6, clarification round 1).

## The order inside the pass

The host facts stand settled once ahead of the device loop
(`provider.cpp:803-812`, the caches `hostCounterLimitRegisters` at `:101-106` and
`hostWatchdogValue` at `:111-115`). For one device the pass reads in this order:

```text
1. decode   detail::limitForDevice(device)          unchanged rule
2. guard    decode answered a value -> publish it, record Decode, pay 0 opens (FR-030)
3. guard    !detail::isProbeCoreDevice(device)      -> publish nothing, pay 0 opens (FR-032)
4. event    detail::probeEventForDevice(device, vendor) -> no event: publish nothing,
            pay 0 opens (FR-034)
5. probe    detail::probeGroupLimit(options)        once per device per process (FR-022)
6. publish  a count answers the seed; the status arms answer no value (FR-018)
7. record   detail::recordLimitSource(path, source) beside the published figure (FR-043)
```

`options.pmuType` carries the device's own type number, the same value the encode path
reads from `/sys/bus/event_source/devices/<dev>/type`, and `options.eventConfig` carries
what step 4 answered. Step 3 and step 4 run before any open, so an uncore device and a
device with no listed event pay nothing at all.

## The one probe per device

```text
\post a device's search runs at most once per process, and a second enumeration of the
     same device reads the settled answer with 0 opens (FR-022, SC-005)
```

The cache keeps the shape of the two caches above: a function-local static map keyed by
device name, filled at the first search, read by every later pass. It is process state of
the provider, and `enumerate` is the only writer.

## The watchdog rule, in two halves

`detail::usableCounters` (`counter_limits.cpp:319-343`) takes one counter off a decoded
figure when the NMI watchdog holds one. That deduction belongs to the decode: the
watchdog occupies a general counter of the decoded set.

```text
\post a decoded value keeps the deduction of `usableCounters` unchanged (FR-044)
\post a probed value reaches the seed as the probe answered it, with no deduction (D-8,
     FR-042)
```

A search asks the kernel whether k copies ran, and the answer already holds whatever the
watchdog took (F-1 to F-4). A second deduction would subtract a counter the kernel
already refused, and the plan would seat fewer leaves than the device schedules.

## The source beside the figure

`recordLimitSource` is the internal recorder of `limit-source.md`, and the provider is its
only writer. The rule a reader can rely on:

```text
\post objectLimitSource(path) answers None exactly where Object::programmableCounters()
     answers no value (FR-040a, FR-043)
```

The provider records `Decode` where step 2 published, `Probe` where step 6 published, and
records nothing where it publishes no value. A device the pass never seats reaches no
record.

## Verification

| Claim | Machine |
|---|---|
| the seven steps reach a case with no host and no kernel interface | `ctest --test-dir build -R counters_limit_fill_test` |
| the decode path pays 0 opens | the same unit, with the seam's open counter at 0 (FR-030) |
| one search per device across two enumerations | the same unit, open ordinals (FR-022, SC-005) |
| a watchdog of 1 costs a decode one counter and a probe nothing | the same unit, scripted registers (FR-020, FR-042) |
| the fill and the reader agree on every scripted device | the pair assertion of T042 (FR-040a) |
| the dark-branch rows of `provider.cpp` hold at their base figure | `tools/dbc/coverage_gate.sh` on the lane host, beside `results/coverage-base.txt` |

## Amendment, round 2 (2026-11-05)

The pass moved out of `provider.cpp` into one function that takes the decode's figure as
an argument: `detail::fillDeviceLimit(device, vendor, watchdogValue, hasFixedCycles,
decoded)`. The reason is a test one. `readCounterLimitRegisters()` executes CPUID
directly (`counter_limits.cpp:148-171`), so a row of this table that waits on the decode
waits on the CPU of the machine running the test, and a row no CI host can reach is a row
no one has tested. `provider.cpp` reads the registers and hands the figure over; the pass
decides, records, and caches.

Step 4 changed with FR-033: the event rule answers the name `branch-instructions`, and
the search opens the words of that entry from the device's own listing on the device's
own type number. A device listing no such entry pays no open (FR-034).

The record joins on every arm, and `NONE` erases, so a stale finder never stands beside an
absent figure (FR-040a).
