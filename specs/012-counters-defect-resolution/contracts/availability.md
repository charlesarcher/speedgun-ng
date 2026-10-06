# Contract: availability and supported targets

Governs `include/speedgun-ng/counters_core.hpp`. Requirements: FR-001,
FR-010, FR-018, FR-021, FR-022.

## `enum class availability`

**Doxygen**: each enumerator names the condition it reports. The
enumeration holds the countability state of one catalog entry, and its
values separate a refusal caused by the entry's scope from a refusal
caused by its encoding.

**Enforcement**: `SG_REQUIRE` in the probe and in the resolution path
that the state published for a slot matches the state the entry
published at registration. A mismatch aborts; the library does not
substitute a state.

| Enumerator | Reports |
| --- | --- |
| `countable` | the entry can be counted on the targets its mask names |
| `permission_blocked` | the kernel refused the event for want of permission |
| `not_encodable` | the running kernel's formats lack a field the row needs |
| `absent` | the host publishes no such event |
| `scope_refused` | the entry's own scope refuses the requested target kind |
| `gap` | one sampling action measured nothing; the catalog never publishes it for an entry |

**Caller reads**: the state of one entry for one target kind. A caller
that sees `scope_refused` knows a cpu-target plan may still compile over
the entry. A caller that sees `not_encodable` knows no target compiles.
A caller never sees `gap` in an entry, because a gap is a property of
one action and an entry spans many actions. A caller reads `gap` in the
plan's disclosure column, which carries it beside the zero count that an
action which measured nothing produced (FR-007).

## Target-kind bitmask

**Doxygen**: a fixed-size bitmask over `target_kind` naming the targets
an entry can be counted on. The type allocates no memory, so reading
the targets costs no allocation. A new kernel target takes the next
free bit; no enumerator value changes and no stored bit moves.

**Type**: a fixed-size unsigned integer. Bit 0 is `target_kind::thread`
and bit 1 is `target_kind::cpu`.

**Enforcement**: `SG_ENSURE` in the probe that the mask holds a bit for
a target kind only where that probe returned a state other than
`scope_refused`.

**Caller reads**: the supported targets beside the state, with no
container and no allocation.

## `struct catalog_entry`

**Doxygen**: one named counter on one object as the catalog reports it.
Views into provider-owned or system-owned storage. The catalog strings
are immutable once the system is open.

**Enforcement**: `SG_REQUIRE` on the resolve path that a returned entry
belongs to the object the caller named, and `SG_ENSURE` that the entry's
`avail` and `targets` agree: a state of `countable` names at least one
target bit.

**Fields this feature changes**: `avail` gains the `scope_refused`
value and the per-action `gap` value; `targets` is a new field beside
it. `name`, `description`, `unit`, `mode`, `frequency_hz`, and
`scaled` keep their meaning. No existing signature changes (FR-024).

## Read mode per entry

`read_mode` names the mechanism a plan reads for an entry. The catalog
publishes the syscall read mode on an entry whose event the fast
instruction cannot read, and one entry's refusal never changes another
entry's mode (FR-001).

**Enforcement**: `SG_REQUIRE` in the window opener that every leaf of a
fast window carries `read_mode::fast_rdpmc`, which is the existing check
behind `all_fast` in `source/counters/linux_pmu/group_io.cpp`.

## Concurrent resolution

Resolving an object, listing objects, and reading an object's parent and
children are safe from any number of threads at once after the catalog
is open (FR-010).

**Enforcement**: `SG_REQUIRE` at the open boundary that the system
reached its open state, and the open flag is written once there. No
contract check guards the maps themselves; the lock does.

## Plan compile

Compiling a plan writes no shared state after the catalog is open, and
is safe from any number of threads at once (FR-011).

**Enforcement**: `SG_REQUIRE` that the catalog is open before a compile
resolves any leaf, and `SG_ENSURE` that a successful compile returns a
plan whose recorder arena is owned by that plan alone.
