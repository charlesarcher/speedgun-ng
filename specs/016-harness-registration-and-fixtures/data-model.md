# Data Model: Harness Registration and Fixtures

Phase 1 output for `specs/016-harness-registration-and-fixtures/spec.md`.
The entities are the Key Entities of the spec plus the records the
expansion and the run move between them.

## E-01: Family record (the registry entry, extended)

The registry holds one family record per registration (`internal.hpp:45`);
`RegistryEntry` (`internal.hpp:31-42`) is that record (R-01).

| Field | Type | Source |
| --- | --- | --- |
| `name` | `std::string` | H1, `internal.hpp:34`; the family name |
| `callable` | `std::function<void(State&)>` | H1, `internal.hpp:35` |
| `minTimeNs`, `warmupTimeNs`, `repetitions`, `fixedIterations`, `metrics`, `runStarted` | run-control optionals, metric seeds, `bool` | H1, `internal.hpp:36-41`; `runStarted` is set at `runner.cpp:141` |
| `args` | argument lists, each `std::vector<std::int64_t>` | H2, R-01; the family calls of FR-001 and FR-002 append them |
| `argNames` | `std::vector<std::string>` | H2, R-01; the labels of FR-001 (E-04) |
| `rangeMultiplier` | `std::int64_t` | H2, R-01; default `kDefaultRangeMultiplier` (E-05, R-17) |
| `setup`, `teardown` | `std::function<void(State&)>` | H2, R-01; one callback per slot (E-07, R-10) |
| `fixtureFactory` | type-erased factory of one fixture object per run | H2, R-01; null for a plain family (E-06) |

Relationships: the `BenchmarkHandle` (`benchmark.hpp:450`) points at one
family record through `m_entry` (`benchmark.hpp:542`); a family call
mutates the record and returns the handle for chaining.

Validation: a family call, `setup`, or `teardown` after the run starts
is a contract violation (`registry.cpp:55, :66, :76, :86, :97`). FR-006
adds `SG_REQUIRE` preconditions (R-15): a multiplier below 2, a low
bound above a high bound, a `denseRange` step below 1, a count mismatch.

## E-02: Instance record

`expandRegistry()` runs at the start of `speedgunMain` (`cli.cpp:142`),
before the filter (`cli.cpp:227-248`) and the first run, and expands
each family record into its instance set (FR-003, R-03).

| Field | Type | Source |
| --- | --- | --- |
| `name` | `std::string` | H2, R-02; the expanded instance name (E-08) |
| `suite` | `std::string` | H2, R-02; the suite split of FR-021 (E-08) |
| `caseName` | `std::string` | H2, R-02; the case split of FR-021 (E-08) |
| `arguments` | `std::span<const std::int64_t>` | H2, R-02, R-04; the instance owns the storage |
| `family` | pointer to the family record | H2, R-02; the E-01 record |

Relationships: the instance list groups by suite in first-appearance
order, and the runner walks it (R-12).

Validation: two instances with one name are a recoverable error found
at expansion (FR-012). The harness prints one standard-error line
naming both instance names, keeps the earlier instance, drops the
later, and leaves the exit status unchanged (Q-7). A family above
`kMaxFamilySize` draws one warning naming the bound of 100 instances;
the instances stay and the run continues (FR-007, R-14). The suite and
case pair is unique per instance (FR-021).

## E-03: Argument list and the state argument view

`State` (`benchmark.hpp:140`) gains the read-only span `m_arguments`
beside the H1 members at `benchmark.hpp:400-408` (R-04). The instance
owns the storage, so a read allocates nothing (FR-008).

| Field | Type | Source |
| --- | --- | --- |
| `m_arguments` | `std::span<const std::int64_t>` | H2, R-04; fixed at expansion, unchanged through a run |
| `range(std::size_t) -> std::int64_t` | method | H2, FR-008; carries `SG_REQUIRE(index < size)` (R-04) |
| `rangeCount() -> std::size_t` | method | H2, FR-008 |
| `createRange`, `createDenseRange` | free builders returning `std::vector<std::int64_t>` | H2, FR-002, R-15; namespace `sg` |

Validation: `range(index)` with an index at or above `rangeCount()` is
a precondition violation (R-04). A family with no family call has one
instance with zero arguments, so `range(0)` is a violation (spec edge
case). The builders carry `SG_REQUIRE` preconditions for a multiplier
below 2, a low bound above a high bound, and a step below 1
(FR-006, R-15). The timed loop keeps the H1 shape (FR-009, R-06).

## E-04: Argument label set

The labels come from `argName` and `argNames` (FR-001) and live in
`argNames` on the family record (R-01), one label per argument position.

| Field | Type | Source |
| --- | --- | --- |
| label list | `std::vector<std::string>` | H2, R-01; `argName` appends one label, `argNames` appends the list |
| segment form | `label:value` | FR-010; a segment carries the label where one is set |

Validation: a label list whose length differs from the family arity is
a precondition violation (FR-006), the check at
`benchmark_register.cc:315-324`. An empty label leaves its segment
without a label (spec edge case).

Relationships: the labels feed instance naming (E-08) at expansion.

## E-05: Range multiplier

One multiplier stands per family record (R-01). The default is 8,
spelled `kDefaultRangeMultiplier` on the shape of `kDefaultMinTimeNs`
(`runner.cpp:22-24`) under N-8 (R-17).

| Field | Type | Source |
| --- | --- | --- |
| `rangeMultiplier` | `std::int64_t` | H2, R-01; default 8 (FR-004) |
| growth rule | bounds inclusive, powers of the multiplier strictly between, negatives mirrored | FR-004; the cited revision at `benchmark_register.h:61` |

Validation: a multiplier below 2 is an `SG_REQUIRE` precondition
violation (FR-006). `rangeMultiplier` sets the multiplier for later
range calls of that family (FR-004), and `createRange` takes
`kDefaultRangeMultiplier` as its default (R-15).

## E-06: Fixture and its setUp and tearDown pair

`sg::Fixture` is the public base class of FR-015. The seven fixture
macros of FR-016 attach a derived class method to the family record.
The class name becomes the suite (R-08).

| Field | Type | Source |
| --- | --- | --- |
| `setUp(State&) -> void` | virtual with an empty default | FR-015 |
| `tearDown(State&) -> void` | virtual with an empty default | FR-015 |
| fixture factory | type-erased callable on the family record | R-01; built by the macros of FR-016 |
| object lifetime | one object per run | R-09; built inside `sampleRun` (`runner.cpp:317-366`) before `setUp`, destroyed after `tearDown` |

Validation: `setUp` runs before the timed loop and `tearDown` after it,
once per run (FR-017). The warm-up, calibration, and measured runs each
get a fresh object (`runner.cpp:373-395`). The state handed to the pair
follows the FR-018 callback rule (R-05). Generated identifiers follow
the `SG_BENCHMARK` shape (`benchmark.hpp:582-593`), with no leading
underscore and no `__` (R-13).

Relationships: the pair wraps the callable with the callback pair
(E-07): setup, `setUp`, callable, `tearDown`, teardown (R-10). The
class name is the suite (E-08).

## E-07: Setup and teardown callback pair

The handle accepts the pair through `setup` and `teardown` (FR-018);
each writes one slot on the family record (R-01). The last attachment
wins (R-10).

| Field | Type | Source |
| --- | --- | --- |
| `setup` | `std::function<void(State&)>` | H2, R-01; runs before the callable of each run |
| `teardown` | `std::function<void(State&)>` | H2, R-01; runs after the callable of each run |
| callback state | harness-built `State&` carrying the instance arguments | FR-018; no new public view type |

Validation: each callback runs once around each run, the warm-up,
calibration, and measured runs included, in the untimed region
(FR-018). Reading the instance arguments and the iteration count is
legal; `begin()`, `skipWithError`, and `skipWithMessage` are
precondition violations there (R-05). `end()` stays legal: static at
`benchmark.hpp:277`, it returns `Cursor {0}` and touches no state.
FR-018 names four violations, and the design enforces three.

## E-08: Instance name with its suite and caseName split

The expanded instance name is the family name plus one `/`-joined
segment per argument (FR-010). The suite and case derive at expansion
(R-08).

| Field | Type | Source |
| --- | --- | --- |
| instance name | `std::string` | FR-010; a segment carries `label:` where E-04 sets one |
| name forms | `fn/captureName`; `fn<` plus the stringified type list plus `>`; `FixtureClass/Method` | R-13; FR-013, FR-014, FR-016 |
| `suite` | `std::string` | R-08; the family name up to its first `/`, the fixture class name for a fixture instance (FR-021, Q-1) |
| `caseName` | `std::string` | R-08; the instance name with the leading suite and its `/` removed |

Validation: an instance name equal to its suite keeps that name as its
case (R-08). The suite and case pair is unique per instance (FR-021,
FR-012).

Relationships: the name is the filter and list-mode key (FR-011) and
the E-10 prefix-test target; the split reaches the result value (E-09).

## E-09: BenchmarkResult with the two new fields

`BenchmarkResult` (`benchmark.hpp:116-125`) is the result value of
FR-021 and FR-026 (R-07). `ResultRow` (`benchmark.hpp:101-109`) carries
neither new field (R-07).

| Field | Type | Source |
| --- | --- | --- |
| `name`, `outcome`, `rows`, `reason`, `metricLabels` | H1 fields | H1, `benchmark.hpp:118-125`; unchanged, `name` carries the full instance name |
| `suite` | `std::string` | H2, R-07; the suite of the instance (E-08) |
| `caseName` | `std::string` | H2, R-07; `case` is a C++ reserved word, and N-12 makes the member camelBack and prefix free |

Validation: the console row keeps the H1 single name column, and the
split is readable on this value (FR-021, R-12). `printResult`
(`report.cpp:42`) dispatches rows on `row.kind` (`report.cpp:57`) and
keeps that shape. The layout change raises the harness `SOVERSION` from
2 to 3 (FR-026, R-16). The roadmap JSON report reuses the two fields
(FR-021).

## E-10: The DISABLED_ name rule

The rule is a gate over the instance name, held at selection in
`cli.cpp` (R-11).

| Field | Type | Source |
| --- | --- | --- |
| prefix | the literal `DISABLED_` | FR-020; a string literal, outside the V.1 identifier scope |
| test target | the expanded instance name | R-11; tested at selection time |
| selection and list effect | the filter skips the instance, list mode omits it | FR-020; `cli.cpp:243-247`, `cli.cpp:250-255` |

Validation: a `DISABLED_` family registers and never runs (FR-020). A
fixture method named `DISABLED_x` keeps its class prefix in the instance
name, so it runs (R-08, R-11). A `DISABLED_` family as the only filter
match draws the H1 no-match report and a zero exit (015 FR-036, spec
edge case).

## Relationships

```text
registry 1──* family record (E-01) 1──1 BenchmarkHandle
family record 1──* instance (E-02) 1──1 BenchmarkResult (E-09)
family record 0..1──1 setup callback, 0..1──1 teardown callback (E-07)
family record 0..1──1 fixture factory 1──1 fixture object per run (E-06)
family record *──1 argument list 1──1 instance argument span (E-03)
family record 1──1 label set (E-04), 1──1 multiplier (E-05)
instance 1──1 name (E-08) 1──1 suite + caseName pair; selection gates by E-10; expansion at speedgunMain start (R-03)
```
