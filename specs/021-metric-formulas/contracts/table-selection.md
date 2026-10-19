# Contract: Table Selection with the Stepping

**Capability**: C-6 host table reachability | **Plan**: D-13 | **Owner**: `source/counters/linux_pmu/table_parse.cpp`, `source/counters/detail/pmu.hpp`

## Surface

```text
mapfileKey(PmuIdent) -> "<vendor>-<family>-<model>-<stepping>"
pmuSelectDirectory(mapping-file stream, PmuIdent) -> directory name
```

Both exist at the audit point. `PmuIdent` carries vendor, family and model;
this feature adds the stepping field beside them. `PmuIdent` lives in
`source/counters/detail/pmu.hpp`, which is private to the library, so no
public declaration moves.

## The defect this closes

The pinned `external/pmu-events/arch/x86/mapfile.csv` splits model `0x55` by
stepping:

```text
GenuineIntel-6-55-[01234],v1.37,skylakex,core
GenuineIntel-6-55-[56789ABCDEF],v1.25,cascadelakex,core
```

The current key omits the stepping, so it reads `GenuineIntel-6-55` and
matches neither row. Replaying all 46 rows of the pinned file against that key
matches nothing, which means a model-55 part selects no vendored directory
today and therefore exposes neither the event table nor the metrics of that
directory. Upstream added the stepping to the CPUID string in 2018 for this
split.

## Guarantees

- The key carries vendor, family, model and stepping in the form the file's
  patterns match, so two CPUs of one model and different steppings select two
  directories (FR-034).
- A fixture for a stepping below the model-55 boundary selects `skylakex` and
  a fixture at or above it selects `cascadelakex` (FR-035).
- Every row of the pinned file that carries a stepping class is matched by at
  least one fixture key, so a re-pin that adds a class fails a test instead of
  selecting nothing in silence (FR-036).
- A row without a class matches the plain key, so the fix leaves the 45 other
  rows behaving as they do now.
- Selection stays a full match against the first column. A prefix rule would
  let the first row of a model win whatever the stepping says, which is the
  defect in another position.

## Matcher rules

| Pattern form | Key form | Match |
| --- | --- | --- |
| `GenuineIntel-6-55-[01234]` | `GenuineIntel-6-55-4` | class holds the stepping digit |
| `GenuineIntel-6-55-[56789ABCDEF]` | `GenuineIntel-6-55-5` | class holds the stepping digit |
| `AuthenticAMD-25-1` | `AuthenticAMD-25-1` | plain equality |
| vendor or family or model mismatch | any | no match, and the next row is tried |

A class holds hex digits, the same alphabet the file uses for the model field,
and the stepping is rendered in that alphabet. An empty class or a malformed
row is skipped the way the loader skips an unparsable row today, with the same
diagnostic path.

## Consequences a reviewer should expect

A model-55 machine that selects nothing today starts selecting a directory.
Its catalog gains the events and metrics of that directory, and the leaves it
offers change. That is the fix working, and `counters_pmu_test.cpp` states it
in both directions. Public headers keep their declarations, so the counters
archive `SOVERSION` stays 2 and the harness `SOVERSION` stays 6.

## Checks

`test/source/counters_pmu_test.cpp` drives the two boundary fixtures and the
class sweep over the pinned file (FR-034 to FR-036), and the metric lookup
test asserts that a host with no selected table reports that it carries no
metric table (FR-017), which stays reachable through a fixture host that
selects nothing.
