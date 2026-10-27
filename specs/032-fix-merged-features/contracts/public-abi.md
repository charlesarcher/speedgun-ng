# Contract: the public ABI, the version lane and the seam

Phase 1 output. The ABI contract of specs 015 and 029 holds; this file
records the lane FR-001, FR-002 and FR-004 fix and the seam FR-021
changes.

## Version lane

| File | Figure | Rule |
| --- | --- | --- |
| `CMakeLists.txt:15` | project version, `0.20.0` today | `0.20.1` with no public-header edit; `0.21.0` with the `SkippedKind` addition (FR-001) |
| `CMakeLists.txt:298` | harness `SOVERSION 12` | `13` only with the `SkippedKind` addition (FR-001) |
| `CMakeLists.txt:80` | archive `SOVERSION 4` | stays `4` (FR-002, D-11) |
| `docs/schema/report-v1.json` | schema `1.5` | unchanged |
| docs samples | the recording build's version | the two hunt samples re-record from the current build (FR-018) |

A `SOVERSION` rises only when that target's public ABI changes, never
goes down, and the commit that moves the project version records the
ABI check beside the move (FR-001, FR-002). The versions agree in each
file the merge touched (FR-004).

## `SkippedKind` (`include/speedgun-ng/hunt.hpp:158`)

- The enum stands public. A new value joins only when an item needs
  one (D-09, FR-036), in the same commit as the version move.
- Every kind prints with the spelling `report.cpp availabilityName`
  prints; a new value joins that table in the same commit.

## The online cpu set seam

- `installTestOnlineCpus` and `onlineCpuSet()`
  (`source/harness/topology.cpp:327`): test-only installation, the
  production reader unchanged.
- The unreadable-line fallback answers the process affinity mask, not
  a count (D-02, FR-021). Both bounds, `execution_context.cpp:655` and
  `jitter_probe.cpp:711`, read the set and check membership in it
  (D-03).
- The `FakeProvider` open-failure seam stays public in the counters
  archive (D-11, FR-038), and the seam-publicity evidence stands in
  the record.
- The probe-cpu affinity-mask rule at
  `specs/027-jitter-probe/contracts/cli-options.md:71` stays its own
  rule (FR-014).
