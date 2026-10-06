# Contract: device routing, fast verdicts, and target scope

This contract covers I-02, I-03, and I-04(a). Every declaration below is a
public contract, and each is paired with the enforcement that guards it
(Principle II).

## What this feature changes

1. A device carries its own fast verdict, taken from its own page.
2. A table unit reaches every device instance the kernel's generator names.
3. A device's target scope comes from the kernel's published data or the
   probe, never from its name.

The core routing of spec 012's FR-019 is unchanged. The sampling-time
behaviour of a refused fast read is unchanged.

## The fast verdict

**At the audit point**: `pmu_probe_fast` at
`source/counters/linux_pmu/fast_read.cpp:203-229` probes one core
hardware event. `provider.cpp:690-691` reads that one verdict and
`provider.cpp:733` passes it to `probe_device` for every device.
`entry_read_selection_for` at `provider.cpp:411` then sets the fast mode
on every countable entry. `group_io.cpp:745-780` opens a fast window
whenever every leaf carries the mode, and reaches the group read only
through the three narrowing arms at `group_io.cpp:626-630`,
`group_io.cpp:656-673`, and `group_io.cpp:683-686`. None consults the
device's own page.

**After this feature**: the verdict is per device.

**Contract**: a device's fast verdict is the `cap_user_rdpmc` capability
its own event page publishes, read through that page's bit-width field as
the header directs. An entry whose device page publishes no such bit takes
the syscall group read at compile time.

**Why this is a defect and not a preference**: an uncore or RAPL page
publishes no `cap_user_rdpmc`. Under the host-wide verdict every fast read
of such an entry is refused at sampling time and every sampling action
discloses a gap, so an uncore metric on a fast-capable host reports a
continuous run of gaps where the kernel would have answered.

**Enforcement**: `SG_REQUIRE(device verdict == entry's device verdict)` at
`probe_device`, and `SG_ENSURE(a window's mode is the mode its device's
page published)` at the window open. The existing
`entry_read_selection_for` takes a mode per entry and gains no parameter,
because the mode is now correct where it is read.

## What does not change at sampling time

Spec 012's FR-002 and D-03 hold without change. A fast read refused at
sampling time discloses a gap and issues no syscall read of the same
event, so the sampling action keeps the cost this library publishes for
it. The correction is a compile-time decision; it adds no branch to the
sampling path and moves no published figure.

**Enforcement**: the existing refused-read fixture
(`counters_linux_pmu_seam_test.cpp`, the refusal at `group_io.cpp:420-423`
and the mark at `group_io.cpp:470-473`) is unchanged and still asserts no
syscall read follows a refusal.

## The narrowings that remain

Three arms still narrow a fast window, and this feature adds a fourth.

| Narrowing | Site | Why |
| --- | --- | --- |
| a layout spanning several devices | `group_io.cpp:626-630` | one fast window serves one device |
| a member leaf whose encoding needs a second configuration word | `group_io.cpp:656-673` | the fast path carries one word |
| a member whose open the kernel refuses | `group_io.cpp:683-686` | nothing to read |
| **a device whose page refuses the fast read** | **`group_io.cpp`** | **new, and the defect this contract fixes** |

## Device routing by unit

**At the audit point**: `scope_reaches` at `provider.cpp:367-397` compares
a table's unit against the device name, or against the name `uncore_`
followed by the unit, by exact text after case folding at
`provider.cpp:374-379`.

**After this feature**: the unit resolves through the kernel's generator
unit map, then matches every device whose name the resolved unit reaches,
including a numbered instance.

**Contract**: a unit reaches a device when the kernel's generator would
publish that unit's device. A numeric instance suffix is ignored, so
`CHA` reaches `uncore_cha_0` and `uncore_cha_1` alike.

**Why**: the generator maps a unit to a device name and ignores a numeric
instance suffix. At the audit point no numbered Intel uncore row and no
AMD uncore row reaches a device. The pinned tree carries 48 distinct
units, and the largest unreachable are `CHA` on 7351 rows, `iMC` on 2785,
`DFPMC` on 376, `IMC` on 354, `L3PMC` on 75, and `UMCPMC` on 39.

**Enforcement**: `SG_REQUIRE(a row reaches no device whose class differs
from the unit's class)`, and fixtures named for a numbered cache instance,
a second numbered cache instance, a numbered memory instance, and the
three AMD vendor devices, each asserted to receive the rows its unit
names. A fixture asserts no uncore row appears on a core device.

**Why not one representative instance**: an uncore instance counts its own
socket's controllers. A plan that reads instance zero and reports a
package-wide figure understates it, which is a wrong count under Principle
VI.

## Target scope

**At the audit point**: `provider.cpp:290-291` marks every device whose
path is not `cpu`, `cpu_core`, or `cpu_atom` as device-scoped, by name.
`provider.cpp:175-180` then skips the per-task probe for those entries and
publishes `scope_refused` at `provider.cpp:176`.

**After this feature**: the name rule goes and the probe verdict stays.

**Contract**: a device that publishes no per-task context is
device-scoped, and a device whose per-thread probe succeeds publishes the
thread target bit on its entries. The three core names stay
device-scoped because the kernel publishes no per-task context for them,
not because of their spelling.

**Why**: the `msr` PMU registers a per-task context and accepts per-task
events, and this host publishes it at `/sys/bus/event_source/devices/msr`.
Its three counters are therefore countable on a per-thread target and the
current rule denies a count the kernel would grant.

**Enforcement**: `SG_ENSURE(an entry publishing the thread target bit
names a device whose per-task probe succeeded)`. The probe verdict is the
evidence the catalog publishes, so there is nothing to assert beyond the
link between the two.

**Why not a hard-coded allow-list**: it replaces one name list with
another and ages the same way. Publishing the thread target bit for every
device is worse, because a device with no per-task context cannot deliver
it and publishing it anyway hands the caller a target the kernel refuses.

## The reference host's devices

This host publishes `amd_iommu_0`, `msr`, and the software and tracepoint
devices. It publishes no uncore and no RAPL device, because it is an AMD
Ryzen 9 9950X3D and that part publishes no such PMU. Every uncore fixture
in this contract is therefore synthetic, and every measurement is taken
against a synthetic device tree (012 FR-034).

## Ordering and concurrency

No new shared state is introduced. The fast verdict, the unit map, and the
probe verdict are all resolved during catalog construction, under the same
guard spec 012's D-06 installed. `provider::catalog()` stays `noexcept`
and the resolution stays allocation-free per entry.

**Enforcement**: the thread sanitizer preset covers the catalog tests
under concurrent resolution, and the counters remain embeddable in a
fixed-iteration per-thread loop with no benchmark-framework code
(007 FR-049, 007 FR-050).
