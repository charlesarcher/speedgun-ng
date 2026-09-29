# Contract: System, Time-Stamp Counter Accessor

**Feature**: `008-timestamp-counter` | Namespace `sg::counters` (R-001) | Entities: [data-model.md](../data-model.md)

This file is a delta. It records what 008 adds to the system handle and the
clauses of the merged 007 contract that this feature changes. Every other
clause of [007 system-contract.md](../../007-counters-and-timers/contracts/system-contract.md)
stands as merged and is not restated here, C-SYS-1 through C-SYS-6 among them.
Where the two files describe the same surface, this file governs the
time-stamp counter.

The catalog entry belongs to the provider seam and is specified in
[provider-contract.md](provider-contract.md). This file governs the one thing
the system handle gains: a short spelling for that entry.

## The added member

```cpp
class system {
  [[nodiscard]] auto tsc() const -> std::expected<counter<dim<0, 1>>, error>;
};
```

- **FR-004**: the return type is the type that resolving the entry by name
  returns, so the accessor and `object::counter<dim<0, 1>>("tsc")` are
  interchangeable at the call site. The two name one canonical entry,
  `machine/tsc`, and one dimension.
- **FR-005**: the call is a tree lookup. It reads no hardware, compiles no
  plan, and mints no recorder.
- **FR-006**: the call does not open the registration boundary, so a program
  may call it and then register a provider. The accessor therefore never
  calls the open transition that `system::object()` and `system::objects()`
  share, which is the transition that closes registration (FR-009).
- **FR-007**: with no clock provider registered the call returns a
  recoverable error naming the absent provider, and returns no counter.
- **FR-008**: on a build that does not execute the time-stamp instruction the
  call returns a recoverable not-present error naming the absent counter, and
  performs no read.
- **FR-010**: the member name reuses `tsc`, vocabulary 007 already declares.
  This feature adds no public token.
- **No export macro.** `SPEEDGUN_NG_EXPORT` is class-level, declared once on
  `class SPEEDGUN_NG_EXPORT system` at
  `include/speedgun-ng/counters_system.hpp:183`, and is never written per
  member. The new member carries none.
- **No build wiring.** `CMakeLists.txt:658` globs `source/counters/*.cpp`
  with `GLOB_RECURSE` and `CONFIGURE_DEPENDS`, and
  `cmake/install-rules.cmake:15` installs the whole `include/` directory. 008
  adds no source file and no header, so neither file changes.

### Dox shape

The declaration carries `@brief`, a prose body, and then `\pre` and `\post`,
each on its own line:

```cpp
  /**
   * @brief Resolves the time-stamp counter entry (FR-004).
   *
   * ... prose ...
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto tsc() const -> std::expected<counter<dim<0, 1>>, error>;
```

The prose body states what the entry carries and where the two recoverable
errors come from. The two contract lines come last, one per line, with a
blank doxygen line before `\pre`. "Pairing rules" says why each line stands
alone.

## Pairing rules (FR-009)

`tools/dbc/dbc_pair_gate.py` matches documented contracts against runtime
enforcement by qualified function name across `include/` and `source/`, and
both drift directions fail the gate.

| Documented | Enforced | Verdict |
| --- | --- | --- |
| `\pre` with an `SG_REQUIRE` in the body | paired | pass |
| `\pre none` with an `SG_REQUIRE` in the body | `enforced-not-documented` | fail |
| a real `\pre` with no `SG_REQUIRE` | `documented-not-enforced` | fail |
| `\pre none` with no `SG_REQUIRE` | paired as an empty set | pass |

The rules that follow decide a pass or a fail on this member.

1. **The gate is bidirectional.** A `\pre` documented without a check fails,
   and a check present without a documented `\pre` fails. One side alone never
   passes.
2. **Each contract line stands alone.** The doc scanner reads line-oriented
   markers. A `\pre` buried mid-sentence in prose is invisible to the gate and
   counts as missing. The same holds for `\post`.
3. **Enforcement sits lexically inside the method body.** The scanner
   attributes a macro to the innermost region containing it. An `SG_REQUIRE`
   in an anonymous-namespace helper called by the accessor attributes to that
   helper, and the accessor reads as unenforced.
4. **The FR-008 check is a plain `if`.** The absent-counter case is a
   recoverable error a caller may handle, so it returns `std::unexpected` and
   carries no `SG_REQUIRE`. A check macro there would demand a documented
   `\pre` that a platform condition cannot state.
5. **The FR-007 check is a plain `if` on the same grounds.** An unregistered
   provider is a recoverable error, and the registration boundary stays open.

## Recoverable error shape

The two recoverable errors follow the house shape fixed at
`source/counters/system.cpp:367` to `:383` for a failed object lookup and
`source/counters/system.cpp:277` to `:365` for registration:

```cpp
return std::unexpected(error {.message = ..., .suggestions = {...}});
```

- The message is lowercase, quotes the failing input, and ends with the
  requirement id in parentheses, as
  `"provider registration after the system opened (FR-009)"` does.
- The FR-008 message names the absent counter. That text lives in
  `source/counters/system.cpp` and in no header, because the header purity
  gate scans every `counters*.hpp` (see "Vocabulary purity") and the counter
  name in the message would read as a platform token there. The FR-007
  message names the absent provider and carries the same restriction for the
  same reason.
- Suggestions follow the 007 rule: at most five entries, catalog names within
  edit distance two first, then descriptions sharing a word, catalog order
  breaking ties. An absent counter with no near-miss returns an empty list.

## Vocabulary purity (FR-010)

`test/counters_header_purity.sh` scans every `counters*.hpp` under
`include/speedgun-ng/` for the substrings `perf_event`, `clock_gettime`,
`rdpmc`, and `rdtsc`. It also scans five core headers for the standalone
uppercase acronym `PMU`: `counters_core.hpp`, `counters_measurement.hpp`,
`counters_provider.hpp`, `counters_system.hpp`, and `counters.hpp`.

The token `tsc` is a term the scan declines to check, recorded at
`test/counters_header_purity.sh:16` to `:18`. The reason is FR-023 of 007,
which mandates `fast_tsc` as a read-mode name, so `tsc` is required
vocabulary and a substring scan of it would flag that requirement. The
accessor name is the same token and is legal for the same reason.

Two consequences bind the doxygen text on this member.

- Public documentation says "time-stamp counter" and "instruction". It never
  writes the read instruction's spelling, in prose or in a comment, because
  the scan is a substring match and that spelling is a scanned term.
- Any diagnostic that would name the counter or the provider lives in the
  source file.

The scan covers headers. Specification files under `specs/` sit outside it.

## Conformance list for the delta

| Clause | Guarantee | Requirement |
|---|---|---|
| C-SYS-7 | The accessor and the named lookup return one type for one entry | FR-004 |
| C-SYS-8 | The accessor reads no hardware, builds no plan, and mints no recorder | FR-005 |
| C-SYS-9 | The accessor leaves the registration boundary open | FR-006 |
| C-SYS-10 | Both failure modes are recoverable errors in the house shape, never contract violations | FR-007, FR-008 |
| C-SYS-11 | Documented contract and runtime check pair in both directions, with enforcement inside the body | FR-009 |
| C-SYS-12 | The core vocabulary gains no token; the header purity scan stays clean | FR-010 |
