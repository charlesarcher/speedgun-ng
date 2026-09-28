# Contract: Provider

**Feature**: `007-counters-and-timers` | Namespace `sg::counters` (R-001) | Entities: [data-model.md](../data-model.md) E-04

The provider contract is the seam the whole feature hangs on: "the system counts stuff," and a provider says what exists and what the points are. The giraffe example (US5) and the out-of-tree conformance test compile against exactly this surface plus the system contract, with no access to library internals (FR-012).

## The two obligations

```cpp
struct provider_iface {                       // registration base (R-004)
  virtual void enumerate(object_sink&) const = 0;   // objects + catalog entries
  virtual std::unique_ptr<window_reader> open(const leaf_set&, target) = 0;
  // setup-time only: the read path enters a window through the direct-call
  // thunk its constructor installed, so a window installing no thunk reaches
  // read_points through this vtable at one lookup per sampling action
};

struct window_reader {
  // yields, for every managed leaf, one cumulative uint64 point per sampling
  // action, plus the leaf's unit and metadata (FR-011):
  void read_points(point_sink&) noexcept;
};
```

A C++20 concept `provider` names the shape an out-of-tree provider implements; `provider_iface` is the registration base the system stores. Everything below the contract (syscalls, mmap pages, JSON parsing) is provider-private (FR-010).

## Point yield (FR-011)

| Yielded per leaf | Type / shape | Rule |
|---|---|---|
| point | `std::uint64_t` | cumulative at the instant of the action; every leaf kind alike (clock ticks, PMU counts, push loads, honks) |
| unit | catalog unit plus dimension mapping | fixed at registration; dimension check happens at resolution, never per read |
| metadata | description, availability, caveats | caveats include multiplex times where the source has enabled/running pairs (FR-041) |

- Cumulative means monotonic-modular: one provider read action never reports a value that moves backwards non-modularly; a provider-internal backwards jump is a provider contract violation, and a hardware wrap across actions is ordinary physics handled by modular delta (spec edge cases, FR-013).
- `read_points` is the sampling-action primitive: all leaves it fills are read within one action, satisfying the per-column invariant (FR-047).

## Read modes (FR-023)

```cpp
enum class read_mode { fast_tsc, fast_rdpmc, syscall, push_load };
```

- Chosen per leaf at plan compile from probe results, recorded, and disclosed in the catalog. A provider may offer any subset; the achieved mode is reported per entry.
- `syscall` is the universal fallback (group `read()` per PMU leader, vDSO clock reads, plain loads). `fast_tsc` and `fast_rdpmc` belong to the clock and PMU providers respectively; push counters are `push_load` by construction.
- A fast-mode value for an off-CPU multiplexed event is stale; the enabled/running leaves keep every fold's disclosure complete in every mode (US7 scenario 4, FR-041).

## Registration obligations (FR-012, US5)

1. Register objects (kind, structured path, optional platform alias, description) and named counters with descriptions and unit mappings, pre-open.
2. Implement `open` to yield cumulative points for the leaves the system asks it to manage.
3. Report probed availability: `countable`, `permission_blocked`, `not_encodable`, `absent` (FR-006). Described-and-unavailable is a first-class state; user code branches on catalog state alone (FR-007, US5 scenario 4).

No core file changes, no core compile flags, no internal headers. The giraffe example's acceptance (SC-003) is: public headers plus the standard library, link manifest names this library alone, and the source touches no `source/` include (FR-049).

## Built-in providers shipped through this one contract

| Provider | Leaves | Availability behavior | Notes |
|---|---|---|---|
| `clock` | monotonic, thread CPU, process CPU, `tsc` (x86 where calibrated) | always countable, zero privileges | `tsc` discloses calibration provenance and any scaling flag (FR-033, FR-034, R-007) |
| `push` | user hot-path counters | countable; confined to the creating thread | `add(n)` plain non-atomic increment; sample-time read plain load; cross-thread use and decrements are tier-3 violations (FR-035, R-008) |
| `fake` | hand-driven scripts over arbitrary object trees | scripted | deterministic point sequences including crafted `2^64` wraps; the test spine (FR-036, R-009) |
| `linux_pmu` | vendored tables merged with kernel-discovered aliases | probed per entry: `countable` / `permission_blocked` / `not_encodable` (FR-037..FR-039) | Linux-only; absent cleanly elsewhere (FR-042) |

## Fast-mode protocol obligations (PMU provider, FR-040)

For each `fast_rdpmc` leaf the provider's read descriptor implements, in order: sequence-count retry, `cap_user_rdpmc` capability gate, one-based index validity with a stated fallback when the index reports not-allowed, kernel offset adjustment, counter-width masking, and same-thread context binding (R-011). The protocol follows `jevents/rdpmc` from andikleen/pmu-tools with attribution carried in the source.

## Conformance list

| Clause | Guarantee | Requirement |
|---|---|---|
| C-PRO-1 | New count source needs catalog entries plus the window implementation; zero core changes | FR-012, SC-003 |
| C-PRO-2 | Every sampling action yields cumulative `uint64` points plus unit and metadata for every managed leaf | FR-011, FR-047 |
| C-PRO-3 | Availability distinguishes described from countable-now, permission-aware | FR-006, FR-039 |
| C-PRO-4 | Achieved read mode recorded and disclosed per entry; plans compile against achieved mode | FR-023 |
| C-PRO-5 | Core vocabulary and public headers carry no platform terms | FR-010 |
| C-PRO-6 | Multiplex disclosure survives every read mode (enabled/running are ordinary leaves) | FR-041, US7 scenario 4 |
