# Contract: the source of a limit, and the one exported reader

**Feature**: specs/041-counter-probe | **Files**:
`include/speedgun-ng/counters_system.hpp` with `source/counters/limit_source.cpp` and
`source/counters/detail/limit_source_record.hpp` | **Audit point**: `858c55d9`
**Spec**: FR-040a, FR-043, FR-050 to FR-053, D-3, D-3a

## What this contract covers

One figure reaches a reader as a count, and one word says where the count came from. The
word lives inside the counters archive, and the harness, the example and the report writer
sit outside it. This contract fixes what crosses that boundary, what stays inside, and the
two `SOVERSION` readings the choice settles.

## The exported surface, and why a member will not do

`plan.md` section D-3a carries the reads. In short: a harness unit includes no header of
`source/counters/`, `SPEEDGUN_NG_EXPORT` exists only under `include/speedgun-ng/`, the
counters target builds with `CXX_VISIBILITY_PRESET hidden`, `Object::m_node` is private,
and `source/counters/system.cpp:446` stands outside this lane (PC-5). A member of
`Object` therefore reaches no reader of this lane, and an internal record alone leaves
FR-050 unmeasurable. Two declarations cross, and nothing else:

```cpp
/// FR-043, D-3: where one object's published counter limit came from.
enum class LimitSource {
  None,   ///< no limit is published for the path
  Decode, ///< the CPUID or table decode of `counter_limits.cpp` answered it
  Probe,  ///< the group search of `group_probe.md` answered it
};

/// FR-040a, FR-043, D-3: the source of the limit `programmableCounters()` answers for
/// `objectPath`.
///
/// \param objectPath a catalog address, the same text `programmableCounters()` reads
/// \pre none. A path no provider recorded is an answer, not a fault
/// \post the answer is `NONE` exactly where `programmableCounters(objectPath)` answers
///        no value (FR-040a)
/// \post the answer is stable for a process once the catalog of that process is open
[[nodiscard]] SPEEDGUN_NG_EXPORT auto objectLimitSource(std::string_view objectPath)
    noexcept -> LimitSource;
```

The doc gate reads the public headers alone (`tools/dbc/dbc_doc_gate.py:9-12`), so this
declaration needs its `///` block with the tags above, and the pair gate counts it: the
reading moves from `347 interfaces` to `348 interfaces` with 0 gaps
(`results/base-readings.md`).

## The recorder stays internal

```cpp
/// FR-043: record where the limit of `objectPath` came from. The provider of
/// `provider-fill.md` is the only writer, and the test seam clears the record.
auto recordLimitSource(std::string_view objectPath, LimitSource source) -> void;

/// FR-071: clear the record between scenarios of one unit.
auto clearLimitSources() -> void;
```

Both stand in `sg::counters::detail` in a header under `source/counters/detail/`, which
joins no interface row of the doc matrix, the same reading
`specs/046-hunter-topdown/contracts/area-mapper.md` states for its own `detail` pair. A
unit reaches them through a relative include, the way
`test/source/counters_counter_limits_test.cpp:36` reaches its private header.

The store is one process table keyed by the object path, beside the table
`programmableLimitTable()` keeps at `source/counters/system.cpp:187-191` and `:305`. It
keeps that file's idiom for the guard, and `system.cpp` itself takes no edit (PC-5). The
recorder answers `NONE` for a path it holds no record of, and the writer never records a
path whose published limit is absent, which is how the `\post` of FR-040a holds without
either reader asking the other.

## The version readings

The ledger comment at `CMakeLists.txt:131-133` states the rule: a public header change
moves the hand-kept number even where no layout moved and no symbol joined. This change
changes a public header and joins one symbol, so the counters archive moves from 6 to 7
and the ledger comment gains one row naming this spec. The harness archive changes no
exported layout, because the report read-back stays inside the harness (FR-052), so its
number stays 21. T054 of `tasks.md` reads both figures against `results/base-readings.md`
and against whatever master carries at rebase time.

## Verification

| Claim | Machine |
|---|---|
| the three answers, and the empty record | `ctest --test-dir build -R counters_limit_source_test --output-on-failure` |
| the accessor and the limit agree on one path | the pair assertion of T042 (FR-040a) |
| one interface joins the matrix with its tags | `cmake --build build --target dbc-gate`, which answers `348 interfaces, 0 gaps` |
| the archive number moves for the stated reason | the `soVersionNumbers` test and the ledger row (FR-053) |
| no writer outside the provider | `git grep -n recordLimitSource -- source/` names `provider.cpp` alone |

## Amendment, round 2 (2026-11-05)

Two lines of this contract were written before the code settled. The enumerators
spell `LimitSource::NONE`, `LimitSource::DECODE` and `LimitSource::PROBE`: clang-tidy's
`readability-identifier-naming` fixes `ScopedEnumConstantCase: UPPER_CASE` inside
`WarningsAsErrors`, and every closed enumeration of the tree follows it. The report
words of FR-051 stay lowercase (`decode`, `probe`, `none`), because they are bytes of a
document and not names of a C++ entity.

The writer records on every arm, and a record of `NONE` erases. The biconditional of
FR-040a then holds at every moment a reader can look, including after a pass that seats
no figure on a path an earlier pass had filled.
