# Contract: the device rule and the event row

**Feature**: specs/041-counter-probe | **Files**:
`source/counters/linux_pmu/counter_limits.hpp` with `.cpp` | **Audit point**: `858c55d9`
**Spec**: FR-030 to FR-034, FR-044, D-1, D-5, D-9

## What this contract covers

Two questions stand ahead of every open: is this device one the probe may search, and
which event does the search carry. Both answers are pure functions of a name and a
vendor, so a table drives them and no host is needed to prove either. The search stands
in `group-probe.md`, and where the answers are read stands in `provider-fill.md`.

## The pair, and where it sits

The two declarations join the file that already decodes the limit
(`limitForDevice` at `counter_limits.cpp:288-317`), because the rule that says which
device may be searched is the same rule that says which device has a limit to decode.

```cpp
/// FR-031, FR-032, D-1: is `deviceName` a core device the probe may search?
///
/// \param deviceName the device part of a catalog address
/// \post true for `cpu`, `cpu_core`, `cpu_atom` and a name that starts with the
///        `armv8_` prefix `source/harness/hunt.cpp:44` spells `kCorePrefix`
/// \post false for an uncore device such as `amd_l3_0` or `amd_df_0`, for a pseudo
///        device such as `instructions`, and for the empty name (D-1)
[[nodiscard]] auto isProbeCoreDevice(std::string_view deviceName) noexcept -> bool;

/// FR-033, FR-034, D-9: which event does the search of `deviceName` carry?
///
/// \post the x86 row answers PERF_COUNT_HW_BRANCH_INSTRUCTIONS (0x0005)
/// \post an `armv8_` name answers no event at this audit point (D-5, FR-033)
/// \post a device with no row answers no value, and the caller pays no open (FR-034)
[[nodiscard]] auto probeEventForDevice(std::string_view deviceName,
                                       CounterLimitVendor vendor) noexcept
    -> std::optional<std::uint64_t>;
```

The constants are data the file reads. `PERF_COUNT_HW_BRANCH_INSTRUCTIONS` is 0x0005 and
names the branch instructions of the measured thread, the event F-1 to F-5 searched with
(`specs/041-counter-probe/tools/group_probe.c`). No such constant stands anywhere in
`source/` or `include/` at the audit point, so this file opens it once.

## The Arm row, and why it is empty

D-5 rules out an Arm hardware run without a named host. Three reads back that rule at
this audit point: `ls -d /sys/bus/event_source/devices/armv8_*` answers nothing on the
acceptance host, `counter_limits.cpp` holds no Arm mention, and the tree holds no Arm
event table. An invented event number would be a claim no host of this lane can check, so
the row stands empty and the answers follow: `isProbeCoreDevice` says yes to the name,
`probeEventForDevice` answers no event, and FR-034 keeps the device from paying an open.
SC-010's Arm row stays pending with that reason, and a later host that lists the events
fills the row in one place.

## How the caller composes the two

The provider reads the decode first. The probe runs where and only where the decode
answered no value, the device passes the device rule, and the event row answers a config
word (`provider-fill.md`, FR-030). A device the decode answers therefore pays nothing,
and no rule of `limitForDevice`, `usableCounters` or `publishMinusOne` moves with this
pair (FR-044).

## Verification

| Claim | Machine |
|---|---|
| the device rows answer with no host and no kernel interface | `ctest --test-dir build -R counters_counter_limits_test --output-on-failure` |
| the uncore and pseudo names answer false, and `armv8_pmuv3_0` answers true | the same unit, one row per name (FR-031, FR-032) |
| the x86 row carries 0x0005 and the Arm row is empty | the same unit, the event table rows (FR-033, D-5) |
| an empty event row costs the caller 0 opens | `counters_pmu_test` (FR-034, `provider-fill.md`) |
| the decode rules stand where they were | the base scenarios of the same unit, unchanged (FR-044) |
