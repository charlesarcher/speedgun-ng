# Contracts: the `FastContext` move operations

Phase 1 output. The two contract pairs the move constructor and the move
assignment carry, in the doxygen of the declarations in
`source/counters/detail/pmu.hpp` and enforced with `SG_ENSURE` beside the
definitions (Principle II: the header states the contract, the
implementation enforces it, and each contract stands stated in one place,
so no comment copy rides beside a check). The pairs keep the
contract-pair gate green (FR-001, SC-004).

The move operations make the type move-only (FR-001): the copy constructor
and the copy assignment are deleted, so an accidental copy fails at
compile time, which is the static assertion of FR-006. The declarations:

```text
FastContext(FastContext&& other) noexcept;
auto operator=(FastContext&& other) noexcept -> FastContext&;
```

A moved-from value owns nothing: `fd` at `-1`, `map` at `nullptr`,
`mapLength` at `0` (FR-001). `owner` and `pinnedCpu` carry no defined
moved-from value; every use of a moved-from context stands excluded by
contract (spec Assumptions). The declarations stand beside the
destructor in the seam header; the definitions stand out of line in
`fast_read.cpp` beside `fastContextClose`, and the release reaches the
one release path (specs/012 FR-013).

## Move constructor

```text
FastContext(FastContext&& other) noexcept
  \pre none
  \post the constructed value holds exactly the descriptor, the mapping,
        and the mapping length `other` held on entry (FR-001)
  \post `other` owns nothing: `other.fd` stands at -1, `other.map` at
        nullptr, and `other.mapLength` at 0 (FR-001, FR-003)
```

Enforcement, in the body after the transfer, over the named captures the
entry takes (`takenFd`, `takenMap`, `takenLength`, the values of `other`
before the move):

```cpp
SG_ENSURE(fd == takenFd && map == takenMap && mapLength == takenLength,
          "a move construction leaves the value with the descriptor, the "
          "mapping, and the length the source held (FR-001)");
SG_ENSURE(other.fd == -1 && other.map == nullptr
              && other.mapLength == 0,
          "a move construction leaves the source owning nothing "
          "(FR-001, FR-003)");
```

## Move assignment

```text
auto operator=(FastContext&& other) noexcept -> FastContext&;
  \pre the two operands name different values
  \post the descriptor and the mapping the target held on entry stand
        released exactly once, before the target takes anything
        (FR-002)
  \post the target holds exactly the descriptor, the mapping, and the
        mapping length `other` held on entry (FR-001, FR-002)
  \post `other` owns nothing: `other.fd` stands at -1, `other.map` at
        nullptr, and `other.mapLength` at 0 (FR-001)
  \post the returned reference names the target
```

The precondition carries a reason the ordering needs it: the release runs
before the take, so a self-move would re-install resources its own release
already closed. The implementation enforces the precondition with one
`SG_REQUIRE` in the body; no branch exists for the state the precondition
excludes (X.2), and the contract machinery stays outside the coverage
measurement (VI, FR-015).

Enforcement, in the body after the release and the take, over the named
captures the entry takes (`takenFd`, `takenMap`, `takenLength`, the values
of `other` before the release):

```cpp
SG_REQUIRE(this != &other,
           "a move assignment names two different values (FR-002)");
SG_ENSURE(fd == takenFd && map == takenMap && mapLength == takenLength,
          "a move assignment leaves the target with the descriptor, the "
          "mapping, and the length the source held (FR-001, FR-002)");
SG_ENSURE(other.fd == -1 && other.map == nullptr
              && other.mapLength == 0,
          "a move assignment leaves the source owning nothing (FR-001)");
```

The "released exactly once" clause has its observable half in the count
test of FR-006: the test compares the process's own descriptor and mapping
counts around the assignment loop, so a second release of one resource
moves the count the test reads.

## The destructor pair stands as it stands

`~FastContext` already documents and enforces its postcondition, "the
context owns no mapping and no descriptor" (`pmu.hpp`). A moved-from
destruction is the same no-op the unopened context exercises (FR-003),
which the new run-time test of FR-006 drives through the count helpers the
lifecycle test already owns.
