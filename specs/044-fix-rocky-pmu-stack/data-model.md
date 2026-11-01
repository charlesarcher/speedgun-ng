# Data Model: Fix Rocky PMU Stack Smashing and the Harness Jitter Window Flake

Phase 1 output. Two states carry this feature: the `FastContext` value the
Linux PMU provider's fast read owns, and the counting reader the B-2 fix
installs through the reader-install seam. Both are internal. The context
lives in `source/counters/detail/pmu.hpp` behind the private seam header;
the counting reader lives in `test/source/harness_jitter_window_test.cpp`
behind `source/harness/detail/jitter_probe.hpp`. No public header names
either one (FR-008, FR-014).

## Entity map

```text
FastContext (five members) ── the fast-read context of one descriptor and
      │                       one mapped page
      ├── default         ──▶ owns nothing
      ├── open            ──▶ owns one descriptor, one mapping, its length
      ├── test placement  ──▶ owns one descriptor and one test-owned mapping
      ├── move-construct  ──▶ the target takes; the source owns nothing
      ├── move-assign     ──▶ the target releases once and takes; the
      │                      source owns nothing
      └── destroy         ──▶ releases what it holds; a moved-from value
                             releases nothing

CountingReader (the B-2 seam state) ── one delegate window, one atomic count
      └── stands where the probe's `machine/tsc` reader stands (FR-012)
```

## E-01: `FastContext`

The per-thread fast-read context of `source/counters/detail/pmu.hpp`
(specs/012 FR-040). The type is move-only after the fix (FR-001): the copy
constructor and the copy assignment are deleted, so an accidental copy is
a compile-time error, which is the static assertion of FR-006. The move
constructor and the move assignment carry the contract pair of
`contracts/move-contracts.md`.

| Member | Type | Rule |
| --- | --- | --- |
| `fd` | `int`, default `-1` | The event descriptor `perf_event_open` returned. `-1` names no descriptor |
| `map` | `void*`, default `nullptr` | The one page mapped for the descriptor, or the page a test placed. `nullptr` names no mapping |
| `mapLength` | `std::size_t`, default `0` | The length of the mapping at `map`; `0` beside a null `map` |
| `owner` | `std::thread::id` | The thread that opened the context. The two read sites compare the calling thread against it (specs/012 FR-031, FR-040) |
| `pinnedCpu` | `int`, default `-1` | The processor a cpu-target plan bound the context to; `-1` names a thread-bound plan (specs/012 FR-045) |

### The owning state

A context owns a descriptor when `fd` stands at or above zero, and a
mapping when `map` stands non-null, with `mapLength` beside it. The
destructor and `fastContextClose` are the one release path (specs/012
FR-013): the
mapping is unmapped at its length, the descriptor is closed, and each
guard stands alone, so a value that owns nothing releases nothing. The
release reads the mapping address, the length, and the descriptor and
nothing else, so a test-owned anonymous mapping releases exactly as a
kernel-owned one does.

### The moved-from state

After a move operation the source owns nothing: `fd` at `-1`, `map` at
`nullptr`, `mapLength` at `0` (FR-001). `owner` and `pinnedCpu` carry no
defined moved-from value: every use of a moved-from context stands
excluded by contract, so no member needs a value beyond "owns nothing"
(spec Assumptions). Destroying a moved-from value releases nothing
(FR-003), which is the same no-op the unopened context already exercises.

### The transitions

| Transition | Entry | Exit |
| --- | --- | --- |
| default | none | The value owns nothing. The destructor releases nothing (FR-003) |
| open (`fastContextOpen`) | default | `fd` holds the granted descriptor, `map` the kernel's page, `mapLength` the page size, `owner` the calling thread, and `pinnedCpu` the target's cpu or `-1` (specs/012 FR-024, FR-031). A refusal publishes a null pointer, and the partial release runs |
| test placement | default | The test sets `fd`, `map`, and `mapLength` on a default-constructed value; `map` names an anonymous mapping the test created with `mmap` and owns (FR-004) |
| move-construct | a fresh value | The target holds exactly the source's descriptor, mapping, and length; the source owns nothing (FR-001) |
| move-assign | the target holds resources, or nothing | The target's former descriptor and mapping stand released exactly once, then the target holds exactly the source's resources; the source owns nothing (FR-001, FR-002) |
| destroy | any state | What the value holds stands released: the mapping unmapped at its length, the descriptor closed. A moved-from value releases nothing (FR-003; specs/012 FR-015) |

### Two defects this model closes

The double release: the defaulted copy assignment let two values hold one
descriptor and one mapping, and both releases ran (FR-002). The deleted
copy operations and the releasing move assignment remove that state.

The stack address: a test site placed a stack page's address in `map`, and
the destructor's `munmap` addressed the stack (B-1). The test-placement
transition is the rule now: the four sites that put a fake event page in a
context place the page in an anonymous `PROT_READ`, `MAP_PRIVATE` mapping
the test creates, and no site clears the context's mapping before
destruction as a work-around (FR-004).

The partial-open scenario fills a fixed-size array of contexts: each
element is constructed in place or moved into, so every descriptor and
every mapping the scenario acquired stands released exactly once
(FR-005).

### The aggregate note

Declaring the move constructor makes `FastContext` a non-aggregate under
the aggregate rules the project builds with: a rule standing since C++20
([dcl.init.aggr]), so the designated-initializer
form the test sites use at the base stands unavailable after the fix. A
test site fills the members on a default-constructed value. Verified
against the acceptance host's toolchain: `g++ -std=c++23` rejects
designated initializers for a type that declares a move constructor.

## E-02: the counting reader

The B-2 fix's state, in `test/source/harness_jitter_window_test.cpp`,
installed through the reader-install seam (`installTestReader`,
`source/harness/detail/jitter_probe.hpp`, R-02). The seam stands a test
reader in place of the one the probe opens over the `machine/tsc` leaf;
the counting reader delegates to the provider's own scripted window and
counts the probe's own reads, kept apart from the runner's reads (FR-012,
diagnosis-b2.md). A null pointer clears the installed reader, the way the
seam's contract states it (R-02).

| Member | Type | Rule |
| --- | --- | --- |
| delegate windows | one `counters::WindowReader` per probe thread, created on the thread's first read | The scripted `machine/tsc` windows the fake provider's `open` returns. Every read the probe makes through the seam forwards to the reading thread's own window, so the stamps the probe folds ride the scripted series (R-02, FR-039). One window per thread is the shape the probe's default path gives every item: a cpu's gap stays the scripted tail, and one shared window would make the two cpus' gaps measure their interleaving, which the quiet entry's windows must not carry |
| count | one `std::atomic<std::uint64_t>` | The reads the probe cpus made through the installed reader: one increment per `readPoints` call. The runner's sampling reads never reach this count |

Validation: the count is the event the gate waits end on (FR-012). A wait
ends when the count reaches a target the test derives from the per-cpu
pass structure the probe documents, 1,024 reads per cpu per measurement
pass, and the waits serve the entry-level and run-level fold checks. No
sleep, no clock read, no timeout, and no retry joins a wait; the waits
keep their spin bound, so a stalled probe still reports a failure (FR-012,
FR-039). The count covers every probe cpu the run probes, because the
seam stands over the one `machine/tsc` leaf the probe reads.
