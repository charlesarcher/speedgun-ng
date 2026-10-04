# Data Model: Nanosecond Counter and Simulation-Start Marker

**Feature**: `specs/011-nanosecond-counter-ssc-mark/spec.md`
**Date**: 2026-10-03

The feature stores nothing and owns no schema. Its data lives in two places
the library already has: a leaf record inside the counter catalog's tree, and
a translation unit's machine code. The entities below describe those two
things and the public header that names them, because a reader deciding
whether the change is a data-model change needs that answer up front.

## Catalog placement

```text
system::impl::objects["machine"]           tree_node
  |- leaves[0]  machine/monotonic          nanoseconds, read_mode::syscall
  |- leaves[1]  machine/thread_cpu         nanoseconds, read_mode::syscall
  |- leaves[2]  machine/process_cpu        nanoseconds, read_mode::syscall
  |- leaves[3]  machine/tsc                none,       read_mode::fast_tsc
  |- leaves[4]  machine/monotonic_raw      nanoseconds, read_mode::syscall   <- added
  |- leaves[n]  <user-declared push counters>         read_mode::push_load
  `- subtrees   cpu/...                    the PMU subtrees under the same root
```

The root node is created once in `system::system()`, keyed by the canonical
path `machine`. `push_provider::enumerate` seeds the same node with
user-declared counter names, and `fake_provider` refuses the `machine` path
outright. A new leaf therefore carries one collision risk: a user-declared
push counter named `monotonic_raw` would be refused by the same duplicate-name
check that already refuses any other clash, with the tree left unchanged.

## Entities

### Nanosecond-rate counter

The catalog leaf at `machine/monotonic_raw`.

| Field | Value | Source of the value |
|-------|-------|---------------------|
| `address` | `machine/monotonic_raw` | composed by the catalog as path + `/` + name |
| `name` | `monotonic_raw` | the seed entry's name |
| `description` | one sentence naming the clock's rate property | the seed entry |
| `unit` | `nanoseconds` | the existing unit token; `unit_from_token` maps it to `unit::nanoseconds` |
| `avail` | `countable` | the default for every catalog seed |
| `mode` | `syscall` | the existing read-mode enumerator every clock leaf carries |
| `frequency_hz` | `0` | the default; the clock reports a rate, and no sampling-action conversion uses this field |
| `scaled` | `false` | the default |

Relationships:

- **Belongs to** the machine root object of the counter catalog, alongside the
  four existing clock leaves.
- **Read by** `object::counter<time_dim>("monotonic_raw")`, the two-step
  resolution every counter uses: `system::object("machine")` then the leaf
  lookup. No single-call resolver for a full leaf address exists, and this
  feature adds none.
- **Produced by** `clock_provider`, the same provider that produces
  `machine/monotonic`, `machine/thread_cpu`, `machine/process_cpu`, and
  `machine/tsc`.
- **Dimension**: `time`, resolved by `dimension_of(unit::nanoseconds)`, which
  is what makes the leaf interchangeable with `machine/monotonic` at a call
  site.
- **Sampled by** the existing window machinery: the provider's `open` maps
  the address to an internal read kind, the window reader calls the sink, and
  the plan's fold treats the result like any other nanosecond leaf.

Validation rules that apply at registration, all already implemented in
`build_leaves`:

1. The unit token must resolve through `unit_from_token`, or the provider is
  refused. `nanoseconds` resolves.
2. The name must be unique within the seed batch and within the leaves
   already committed to the machine root. A clash refuses the whole provider
   and leaves the tree unchanged.
3. The address is derived, never supplied, so an address collision is a name
   collision.

State: the leaf is immutable after registration. The sampled value is a
cumulative unsigned 64-bit nanosecond count with no per-counter state
anywhere; the platform owns the epoch and the kernel owns the seqlock the
vDSO reads under.

### Simulation-start tag

A named public constant in the new public header.

| Field | Value | Constraint |
|-------|-------|------------|
| type | unsigned 32-bit | matches the marker's immediate operand, `mov r32, imm32` |
| value | `0xFACE` | asserted at compile time to lie within the unsigned 32-bit range |
| form on a tracer command line | `FACE` | most-significant nibble first, no `0x` prefix |
| form in the instruction stream | `CE FA 00 00` | little-endian, bytes 1 through 4 of the marker window |

Relationships:

- **Consumed by** `simulation_start`, which encodes it into the marker.
- **Passed by the caller** to Intel SDE on the tracer's command line, as
  `-start_ssc_mark FACE`. The library neither launches a tracer nor reads a
  tracer's output.
- **Independent of** the counter: no entity, no state, and no test artifact
  is shared between the two halves of this feature.

Validation: a `static_assert` in the marker translation unit. The value is a
compile-time constant, so a caller-invoked macro and a run-time tag are both
absent by construction, which is what FR-020 requires.

### Marker sequence

The eight bytes `BB imm32(LE) 64 67 90`, emitted as one assembly statement.

| Byte | Meaning |
|------|---------|
| `BB` | `mov r32, imm32` with the register field selecting EBX |
| bytes 1 to 4 | the tag, little-endian |
| `64` | FS segment-override prefix |
| `67` | address-size override |
| `90` | the one-byte no-operation |

Relationships:

- **Recognized by** an attached tracer as an eight-byte window at an
  instruction boundary. Bytes outside the window, including a neighbouring
  no-operation, are not part of the marker.
- **Produced by** the marker translation unit and by nothing else in the
  library. The gate's exactly-once assertion relies on the unit holding
  exactly one function, which makes the in-function count equal to the
  object-wide count the script measures.
- **Architecturally inert** except for EBX, which the compiler saves and
  restores because the statement names it in the clobber list.

State transitions: none. The sequence leaves no state a program can observe
after the call returns, and a tracer's reaction is outside the library.

### Codegen gate

A shell script registered as one CTest entry.

| Field | Value |
|-------|-------|
| input | the marker translation unit, compiled by the script |
| compilers | each of `g++` and `clang++` when the script finds it on `PATH` |
| contract settings | `SG_CONTRACTS_SEMANTIC` 0 and 2 |
| optimization | a release level, `-O2`, independent of the build tree's own type |
| assertion | the eight-byte window appears exactly once in the object's text |
| negative probe | a scratch copy with a wrong window must make the assertion fail |
| skip | no compiler on `PATH` targets x86, so the script reports a skip and exits zero |

Relationships:

- **Depends on** the marker translation unit and on the generated export
  header, which the script includes by path, the way the TSC read-shape gate
  already does.
- **Independent of** the counter lane; the two halves of this feature share
  no test artifact and no file.

### Public header

`include/speedgun-ng/simulation.hpp`, a new file in the install tree.

Validation rules that apply to it:

1. The header includes the generated export header and annotates both
   exported entities with `SPEEDGUN_NG_EXPORT`.
2. The vocabulary scan covers it and finds no platform term. Its scanned set
   is widened by X1, because the scan's existing glob matches the counters
   headers by name.
3. Every in-scope function carries a documented precondition and
   postcondition, and `none` is the project's spelling for a contract with
   nothing to assert.
4. Installation needs no build change: the install rule copies the whole
   `include/` directory.

## What this feature does not model

No provider, registration macro, dispatch table, or counter base class
(FR-034). No new enumerator in the unit or read-mode vocabularies (FR-002).
No run-time configuration of the tag (FR-020). No second marker, no region
identifier, and no nesting scheme. No tracer process, no tracer output, and
no tracer dependency (FR-028).