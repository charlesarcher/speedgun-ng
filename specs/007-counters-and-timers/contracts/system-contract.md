# Contract: System, Objects, Catalog, Resolution

**Feature**: `007-counters-and-timers` | Namespace `sg::counters` (R-001) | Entities: [data-model.md](../data-model.md) E-01..E-03

The public interface of the system handle. Every function below carries doxygen `\pre`/`\post`/`\invariant` plus runtime enforcement at implementation (constitution II); this file fixes what those contracts say.

## Handle and lifecycle

```cpp
class system {
  static system& local();                                // process-local root handle
  std::expected<void, error> register_provider(std::unique_ptr<provider_iface>);
  // pre:  system not yet open (FR-009)
  // post: provider's objects merged into the tree; catalog immutable once open
  // error tier: register-after-open is a recoverable error (FR-009)
};
```

- `local()` returns the process singleton; first use opens (registration window closes). Catalog immutability after open makes concurrent reads safe by construction (FR-009, R-015).
- Duplicate object path under one parent, or duplicate counter name within one object, at registration time: recoverable `error` (FR-008 edge; US3 scenario 6). The tree is left unchanged by a failed registration.

## Object access and selection

```cpp
class system {
  std::expected<object&, error> object(std::string_view path);
  // resolves canonical structured path or platform alias to the same object (FR-002)
  // failure: recoverable error, diagnostic carries near-miss names and descriptions (FR-008)

  object_range objects(std::string_view kind, /* attribute filters */ ...);
  // returns exactly the matching objects (FR-003); unknown kind: recoverable error (FR-008)
};

class object {
  std::string_view path() const;          // canonical spelling, always (FR-002)
  std::string_view alias() const;         // empty when the object declares no alias
  std::string_view kind() const;          // FR-001
  std::string_view description() const;   // FR-001
  const object* parent() const;           // null only for machine (FR-001)
  counter_catalog counters() const;       // this object's entries (FR-001, FR-004)
  object_range children() const;          // US3 scenario 1
};
```

- Every API result, provenance line, and diagnostic prints the canonical path; alias strings never appear in output (FR-002).
- `alias()` returns `std::string_view` and yields an empty view for an object that declares no alias (`include/speedgun-ng/counters_system.hpp:49-55`, `source/counters/system.cpp:449-452`). FR-002 requires that both spellings resolve to one object and names no wrapper type, and an empty view already carries the absence, so this artifact follows the shipped accessor: changing a public accessor would touch its documentation contract block and every caller for no behavioral gain.
- Filter keys (FR-003): equality predicates combined with AND. The ancestor selectors `package` and `core` match the corresponding canonical-path components; a provider may declare further attribute keys for the kinds it registers; an unknown filter key is a recoverable `error` (FR-008).

## Catalog enumeration

```cpp
struct catalog_entry {
  std::string_view name;            // unique within the owning object (FR-005)
  std::string_view description;     // FR-005
  unit unit;                        // plus its dimension mapping (FR-005)
  availability avail;               // see below (FR-006)
  read_mode mode;                   // achieved mode where applicable (FR-005, FR-023)
};

enum class availability { countable, permission_blocked, not_encodable, absent };
```

- Described-ness (from data) and countability (from probe, permission-aware) are reported as separate predicates inside `avail` state; user code branches on catalog state alone, with zero compile-time platform branching in the public core (FR-006, FR-007).
- Non-Linux platforms: identical interface, reduced catalog (clocks, push, fake); the empty PMU section is catalog data, branchable (FR-042).

## Resolution

```cpp
class object {
  std::expected<resolved_leaf, error> counter(std::string_view name) const;
  // post: leaf carries name, description, unit dimension, availability (US1 scenario 1)
  // error: near-miss suggestions from catalog names and descriptions (FR-008, US1 scenario 2)
};
```

- Unit-to-dimension mapping is a closed switch; an unrecognized unit produces a resolution error naming the unit, and never a guessed dimension (FR-017, US1 scenario 6).

## Error shape (tier 2)

```cpp
struct error {
  std::string message;                  // what failed, naming the input
  std::vector<std::string> suggestions; // near-miss catalog names/descriptions (FR-008)
};
```

`std::expected` is the carrier (R-002). Tier-1 violations (dimensions) never reach runtime; tier-3 violations (`SG_*`) terminate in dev/CI and never surface as `error` values (FR-046).

Suggestion rule (FR-008): at most 5 entries; catalog names within edit distance 2 of the failed name rank first, then entries whose description shares a word with it; catalog order breaks ties. A resolution failure with no near-miss returns an empty list.

## Vocabulary purity (FR-010)

No public name in catalog, algebra, dimensions, points, folds, provenance, recorder, or plan contains a hardware-counter, clock-syscall, or platform-concept term. `perf_event`, `clock_gettime`, `rdpmc`, `rdtsc` appear only inside provider implementation files. The machine-readable check is the header scan registered in CTest (Test Plan, plan.md).

## Conformance list for callers and providers

| Clause | Guarantee | Requirement |
|---|---|---|
| C-SYS-1 | Path and alias resolve to one object; output spells canonical | FR-002 |
| C-SYS-2 | Selection returns exactly the matches | FR-003 |
| C-SYS-3 | Catalog reports name, description, unit+dimension, availability, achieved mode | FR-005, FR-006 |
| C-SYS-4 | Failed resolution carries suggestions; nothing guessed | FR-008, FR-017 |
| C-SYS-5 | Post-open catalog is immutable and race-free | FR-009 |
| C-SYS-6 | Core vocabulary carries zero platform terms | FR-010 |
