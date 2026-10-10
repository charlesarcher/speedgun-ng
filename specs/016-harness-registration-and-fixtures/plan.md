# Implementation Plan: Harness Registration and Fixtures

**Feature branch**: `016-harness-registration-and-fixtures` | **Spec**: [spec.md](./spec.md)
**Audit point**: `30f3118`, the H1 merge the spec cites. This plan reads the tree at `389cfba`.
**Date**: 2026-10-10 | **Constitution**: 2.18.0 | **Reference revision**: `google/benchmark` `main` at `e662de9a`

## Summary

H2 turns the single registration point of H1 into a family surface. One
registered function stays one family record in the registry. The handle
accumulates argument lists, argument labels, a range multiplier, two
callbacks, and a fixture factory on that record. `speedgunMain` expands
the record into instances before the filter runs, so no expansion work
touches a run. Each instance carries its own name, suite name, case
name, and argument vector, and the runner walks instances in the order
the report prints them.

`State` gains a read-only argument view: `range(index)` and
`rangeCount()` read a span the instance owns, so a read allocates
nothing. `sg::Fixture` contributes `setUp` and `tearDown` around every
run, and the handle contributes the setup and teardown callback pair.
Capture and template macros reach the same registry through new `SG_`
macros. The result value gains the suite and case fields, and that value
is where the split becomes readable.

Two spec statements need a design call, and this plan records both. The
callback-state rule of FR-018 covers three of the four methods it names,
because `State::end()` is static and cannot see the state it was called
on (R-05). The result value of FR-021 and FR-026 is
`sg::BenchmarkResult`, and the two new fields go there beside `name`
(R-07).

## Technical Context

**Language/Version**: C++23, the pinned standard of Principle I. No
compiler extension enters (FR-028).

**Primary Dependencies**: the H1 harness at the audit point (the
registry, the runner, the report, the filter, list mode, and the timed
window), the counters library, and the counters fake provider
(`include/speedgun-ng/counters_fake.hpp`) as the test substitution.
Google Benchmark is a reference read; nothing links against it.

**Storage**: N/A. The registry and the expanded instance list are
process memory.

**Testing**: CTest executables in `test/`, the fake provider for every
harness test (FR-029), the negative-compile harness for the contract
violations, and the H1 gate scripts `test/loop_shape.sh` and
`test/time_source_gate.sh`.

**Target Platform**: Linux, the CI matrix of Principle VIII: GCC and
Clang, sanitizers, and coverage.

**Project Type**: library plus the harness archive the library exports.
No new target.

**Performance Goals**: one argument read costs one span index and no
allocation (SC-002); the timed loop keeps the H1 shape (FR-009); a run
keeps its two samples per repetition.

**Constraints**: single-threaded (FR-019); expansion completes before
the filter and the first run (FR-003); a family above 100 instances
warns and keeps its instances (FR-007); every time and counter value
comes from counters (FR-025).

**Scale**: 30 requirements, 10 capabilities, one public header, five
harness sources, one new harness source, eight new test executables, one
new documentation page, and one example edit.

## Constitution Check

*(Gate: principles I to XI of 2.18.0 pass. Any **NO** needs an entry in
Complexity Tracking.)*

| Principle | Verdict and note |
| --- | --- |
| I | PASS. C++23 throughout. No new non-standard construct, so no new P2. `std::span` carries the argument view (R-04). |
| II | PASS with one recorded narrowing. Every new interface documents `\pre`, `\post`, and `\invariant` and enforces them through `SG_REQUIRE`, `SG_ENSURE`, or `SG_ASSERT` (FR-024). `State::end()` is static and cannot enforce the FR-018 precondition, so the rule covers `begin()`, `skipWithError`, and `skipWithMessage` (R-05). |
| III | PASS. Requirements, design, code, and unit tests each carry their artifact. `plan.md` records TDD mode (FR-029). |
| IV | PASS. The new public declarations carry doxygen blocks, and `docs/pages/harness.md` holds one section per capability (FR-027). |
| V | PASS. Every new identifier follows N-1 to N-12, every new macro takes the `SG_` prefix, and the name check reports zero findings (FR-023). The result pair spells `suite` and `caseName`: `case` is a reserved word (R-07). |
| VI | PASS. Each capability C-1 to C-10 gains its own test executable, and the 100% line, branch, and DBC gates stay measured on the same paths. |
| VII | PASS. The critical path stays the timed loop. The argument read allocates nothing, and the loop-shape gate stays green (FR-009, R-06). |
| VIII | PASS. No gate weakens and no gate is added, so no amendment is needed. The version and `SOVERSION` table follows FR-026 (R-17). |
| IX | PASS. Spec, plan, and tasks each live in `specs/016-harness-registration-and-fixtures/`. |
| X | PASS. No thread support, no user counters, no JSON report, and no statistics enter: each is a roadmap spec (Out list of the spec Scope). The `args` overload pair stands on the two input forms FR-001 and FR-002 name. |
| XI | PASS. The artifacts and the code prose obey XI.1 to XI.7, and the prose gate reports zero findings (FR-030). |

**Gate weakening requested**: no.

## Project Structure

### Documentation (this feature)

```text
docs/
├── pages/
│   ├── barrier-rule.md            # unchanged (015)
│   └── harness.md                 # Added: one section per capability C-1 to C-10 (FR-027)
└── Doxyfile.in                    # unchanged
```

### Source (this feature)

```text
include/speedgun-ng/
├── benchmark.hpp                  # Edited: family calls, the argument view,
│                                  #   sg::Fixture, the new macros,
│                                  #   BenchmarkResult suite and case fields
└── counters*.hpp                  # unchanged

source/harness/
├── detail/internal.hpp            # Edited: the family record fields, the
│                                  #   instance record, the expansion entry point
├── family.cpp                     # Added: expansion, instance naming, suite and
│                                  #   case derivation, the duplicate and size checks
├── registry.cpp                   # Edited: the family record gains its new fields
├── runner.cpp                     # Edited: run one instance; the callback and
│                                  #   fixture pair inside the untimed region
├── report.cpp                     # Edited: fill the suite and case fields
├── cli.cpp                        # Edited: expand at the start of speedgunMain,
│                                  #   then select, filter, and list instances
└── catalog.cpp                    # unchanged

CMakeLists.txt                     # Edited: version 0.7.0, harness SOVERSION 3
test/
├── CMakeLists.txt                 # Edited: eight new harness test executables
├── source/harness_family_test.cpp         # Added: C-1
├── source/harness_instance_name_test.cpp  # Added: C-2 and C-9
├── source/harness_argument_test.cpp       # Added: C-3
├── source/harness_capture_macro_test.cpp  # Added: C-4
├── source/harness_template_test.cpp       # Added: C-5
├── source/harness_fixture_test.cpp        # Added: C-6
├── source/harness_callback_test.cpp       # Added: C-7
├── source/harness_disabled_test.cpp       # Added: C-8
├── compile-fail/                  # Edited: the callback-state and family
│                                  #   argument violations
└── loop_shape.sh, time_source_gate.sh     # unchanged, both stay green
example/
└── benchmark_example.cpp          # Edited: one argument family, one fixture,
                                   #   one templated benchmark (FR-027)
```

No new build target: the harness archive `speedgun-ng_harness` gains
`source/harness/family.cpp`, and the eight test executables link
`speedgun-ng::harness` as the H1 block of `test/CMakeLists.txt:476-535`
does.

## Design

### Logical view: family, instance, and expansion

The registry keeps one record per registration. Expansion is a pure
step from records to instances, and it runs once.

```text
registerBenchmark(callable, "fib/bits")
        |
        v
  RegistryEntry (family record, registration order)
    callable, run-control optionals, metrics      (H1)
    args, argNames, rangeMultiplier               (H2, R-01)
    setup, teardown, fixtureFactory               (H2, R-01)
        |
        |  expandRegistry(): called at the start of speedgunMain (R-03)
        v
  Instance list (flat, suite-grouped, R-12)
    name "fib/bits/1/8/64"   suite "fib"   case "bits/1/8/64"
    arguments span<int64_t>  family*
```

`BenchmarkHandle` points at the family record, so a family call mutates
that record and returns the handle for chaining. `Runner::run` takes an
`Instance` (R-02). Both types stay inside
`source/harness/detail/internal.hpp`, so the public surface gains no
type beyond `sg::Fixture`.

### Logical view: one run, start to finish

`Runner::run` builds one `State` per run inside `sampleRun`
(`source/harness/runner.cpp:317-366`). H2 wraps the callable with the
callback pair and the fixture pair, and both wrappers sit in the untimed
region FR-005 and FR-017 already define (R-10):

```text
sampleRun(iterations)
  State state(iterations, recorder, instance.arguments)   :324
  fixture = family.fixtureFactory()                       added, R-09
  family.setup(state)                                     added, R-10
  fixture.setUp(state)                                    added, FR-017
  entry.callable(state)                                    :326
  fixture.tearDown(state)                                  added, FR-017
  family.teardown(state)                                   added, R-10
  destroy fixture                                          added, R-09
  state.topUpWindow()                                      :336
  FR-017 loop-discipline check                             :342-355
```

The window pair stays exactly two samples per run: the callback and
fixture work runs before `begin()` opens the window and after the loop
closes it. A scripted-clock test observes that their cost never reaches
the reported time (SC-005).

### Logical view: the state argument view and the callback rule

`State` gains `std::span<const std::int64_t> m_arguments` and a
callback-state flag (R-04, R-05). The instance owns the storage, so a
read is one index and no allocation.

| Operation | In a run | In a callback or fixture hook |
| --- | --- | --- |
| `range(index)`, `rangeCount()`, `iterations()` | legal | legal |
| `begin()`, `skipWithError`, `skipWithMessage` | legal | `SG_REQUIRE` violation |
| `end()` | legal | legal: it is static, returns `Cursor {0}`, and touches no state |

The callback state is a harness-built `State` carrying the instance
arguments and no recorder window. FR-018 names four precondition
violations; three are enforceable, and the fourth is unenforceable
because the member is static at `benchmark.hpp:277`. Making it
non-static would replace an H1 signature, which FR-022 forbids. The
narrowing is recorded in Complexity Tracking.

**Loop-shape exposure (R-06).** `test/loop_shape.sh` reads disassembly,
not source. `loop_body` selects the backward jump with the shortest span
in the whole function (`test/loop_shape.sh:96-113`), and `analyze` then
fails on an out-of-line `call` inside that body (`:190`) or on a body
longer than the `referenceLoop` count-down (`:210`). A contract check in
`begin()` runs once per run and sits outside the loop, and `begin()`
already carries one `SG_ENSURE` at `benchmark.hpp:263-264`. The rule for
H2 is the one the gate measures: a new check must not emit a backward
jump shorter than the timed loop. The check is
`ctest --test-dir build -R loop_shape` in a `ci-ubuntu` tree.

### Logical view: names, suites, and cases

```text
family name            instance name                    suite      case
fib/bits          ->   fib/bits/1/8/64                  fib        bits/1/8/64
QueueFixture      ->   QueueFixture/push/8              QueueFixture  push/8
DISABLED_slow     ->   DISABLED_slow/8  (never runs)     -          -
```

The suite is the family name up to its first `/`, and the fixture class
name for a fixture instance (Q-1). The case is the instance name with
the leading suite and its `/` removed. Uniqueness follows from the
unique instance name (FR-012). The console row keeps the single H1 name
column, and the pair reaches the caller on `BenchmarkResult` (R-07).

### Physical view

| File | Namespace | Holds | Public? | Target |
| --- | --- | --- | --- | --- |
| `include/speedgun-ng/benchmark.hpp` | `sg` | `BenchmarkHandle` family calls, `State` argument view, `sg::Fixture`, the new macros, `BenchmarkResult` fields | yes | `speedgun-ng_harness` |
| `source/harness/detail/internal.hpp` | `sg` | the family record fields, `Instance`, `expandRegistry()` | no | `speedgun-ng_harness` |
| `source/harness/family.cpp` | `sg` | expansion, instance naming, suite and case derivation, the duplicate and family-size checks | no | `speedgun-ng_harness` |
| `source/harness/runner.cpp` | `sg` | one run of one instance, the callback and fixture pair | no | `speedgun-ng_harness` |
| `source/harness/report.cpp` | `sg` | the suite and case fields of the printed result | no | `speedgun-ng_harness` |
| `source/harness/cli.cpp` | `sg` | expansion at the start of `speedgunMain`, selection, filter, list mode, the `DISABLED_` rule | no | `speedgun-ng_harness` |
| `test/source/harness_*_test.cpp` | - | the C-1 to C-9 checks | - | eight test executables |

### Public API surface added

```cpp
// BenchmarkHandle, each returning BenchmarkHandle& for chaining
auto arg(std::int64_t value) -> BenchmarkHandle&;
auto args(std::initializer_list<std::int64_t> values) -> BenchmarkHandle&;
auto args(std::vector<std::int64_t> values) -> BenchmarkHandle&;
auto range(std::int64_t low, std::int64_t high) -> BenchmarkHandle&;
auto rangeMultiplier(std::int64_t multiplier) -> BenchmarkHandle&;
auto ranges(std::vector<std::pair<std::int64_t, std::int64_t>> bounds)
    -> BenchmarkHandle&;
auto denseRange(std::int64_t low, std::int64_t high, std::int64_t step = 1)
    -> BenchmarkHandle&;
auto argsProduct(std::vector<std::vector<std::int64_t>> lists)
    -> BenchmarkHandle&;
template <class F> auto apply(F&& fn) -> BenchmarkHandle&;
auto argName(std::string_view label) -> BenchmarkHandle&;
auto argNames(std::vector<std::string> labels) -> BenchmarkHandle&;
auto setup(std::function<void(State&)> callback) -> BenchmarkHandle&;
auto teardown(std::function<void(State&)> callback) -> BenchmarkHandle&;

// State, the argument view
[[nodiscard]] auto range(std::size_t index) const -> std::int64_t;
[[nodiscard]] auto rangeCount() const noexcept -> std::size_t;

// The fixture base
class SPEEDGUN_NG_EXPORT Fixture
{
public:
  virtual ~Fixture();
  virtual auto setUp(State& state) -> void;
  virtual auto tearDown(State& state) -> void;
};

// Free argument-list builders
[[nodiscard]] auto createRange(std::int64_t low, std::int64_t high,
                               std::int64_t multiplier = kDefaultRangeMultiplier)
    -> std::vector<std::int64_t>;
[[nodiscard]] auto createDenseRange(std::int64_t low, std::int64_t high,
                                    std::int64_t step = 1)
    -> std::vector<std::int64_t>;

// BenchmarkResult, the two new fields (R-07)
std::string suite;
std::string caseName;

// Macros (FR-013, FR-014, FR-016)
SG_BENCHMARK_CAPTURE, SG_BENCHMARK_TEMPLATE,
SG_BENCHMARK_F, SG_BENCHMARK_DEFINE_F, SG_BENCHMARK_REGISTER_F,
SG_BENCHMARK_TEMPLATE_F, SG_BENCHMARK_TEMPLATE_DEFINE_F,
SG_BENCHMARK_TEMPLATE_METHOD_F, SG_BENCHMARK_TEMPLATE_INSTANTIATE_F
```

`registerBenchmark`, `SG_BENCHMARK`, `speedgunMain`, the H1 setters, and
the H1 `State` methods keep their signatures (FR-022).

### Version and `SOVERSION`

| Artifact | At audit point | After H2 | Reason |
| --- | --- | --- | --- |
| `project(speedgun-ng VERSION)` | 0.6.0 | 0.7.0 | FR-026, the next minor |
| `speedgun-ng` archive `SOVERSION` | 2 | 2 | the counters surface changes nothing |
| `speedgun-ng_harness` archive `SOVERSION` | 2 | 3 | `State` gains argument storage and `BenchmarkResult` gains two fields, so both layouts change (FR-026) |
| constitution | 2.18.0 | 2.18.0 | no amendment: no gate changes |

## Test Plan

**Execution mode**: TDD (Principle III, FR-029). Each test file is
written and observed red before the implementation that turns it green.

**Scenario and check mapping** (each runs on the fake provider, so no
PMU and no wall-clock dependence):

| Capability | Scenario | Check |
| --- | --- | --- |
| C-1 argument families | every family call of FR-001 and FR-002 against the instance set of the cited revision; `createRange` and `createDenseRange` lists accepted; the multiplier rule; the 100-instance warning | `test/source/harness_family_test.cpp` |
| C-2 instance names | name forms for each family shape, the `argName` label segment, the filter and list mode over instance names, the duplicate at expansion with its standard-error line and unchanged exit status | `test/source/harness_instance_name_test.cpp` |
| C-3 argument access | `range(index)` and `rangeCount()` in every run of an instance; an instrumented read allocates nothing; the timed loop keeps its shape | `test/source/harness_argument_test.cpp`, `test/loop_shape.sh` |
| C-4 capture | `SG_BENCHMARK_CAPTURE` name segment and captured values | `test/source/harness_capture_macro_test.cpp` |
| C-5 templates | `SG_BENCHMARK_TEMPLATE` name stringification and instantiation | `test/source/harness_template_test.cpp` |
| C-6 fixtures | the seven fixture macros, the class name as suite, `setUp` and `tearDown` once per run including warm-up and calibration, scripted clock deltas showing no added time | `test/source/harness_fixture_test.cpp` |
| C-7 callbacks | the callback pair once per run, the order of R-10, the last attachment winning, and the callback-state rule of R-05 | `test/source/harness_callback_test.cpp`, `test/compile-fail/` |
| C-8 `DISABLED_` | a disabled family runs nothing, the filter skips it, list mode omits it, and a fixture method named `DISABLED_x` runs | `test/source/harness_disabled_test.cpp` |
| C-9 suite and case | the suite order of first registration, registration order inside a suite, unique pairs, and the two fields on `BenchmarkResult` | `test/source/harness_instance_name_test.cpp` |
| C-10 examples and docs | the example builds and runs the three new shapes; `docs/pages/harness.md` holds one section per capability | `example/CMakeLists.txt`, prose gate, review |
| whole surface | every hard gate of Principle VIII at the feature head | SC-008 run of the CI matrix |

**Registration pattern**: each new test follows the feature 015 block of
`test/CMakeLists.txt:476-570`: a `harness_<area>_test` executable built
from `source/harness_<area>_test.cpp`, linked to
`speedgun-ng::harness`, `cxx_std_23`, with the CTest name equal to the
target name.

**Gates that must stay green**: `loop_shape`
(`test/CMakeLists.txt:548-549`), `cli_shape` (`:553-554`),
`barrier_shape` (`:541-542`), and `time_source_gate` (`:482-485`, also
run standalone in CI at `.github/workflows/ci.yml:32`). The coverage
target measures 100% line and branch over `include/speedgun-ng/*` and
`source/*` only, and it omits `SG_REQUIRE`, `SG_ENSURE`, `SG_INVARIANT`,
and `SG_ASSERT` lines (`cmake/coverage.cmake:67-70`), so every new
harness branch needs a test while its contract lines stay free. The
presets are `ci-ubuntu` to `build`, `ci-sanitize` to `build/sanitize`,
`ci-tsan` to `build/tsan`, and `ci-coverage` to `build/coverage`; the
`dev` preset is machine-local and CI cannot assume it.

**Name check reach**: `WarningsAsErrors` is exactly
`readability-identifier-naming` (`.clang-tidy:21`), so a naming finding
is a build error. The check resolves no identifier inside a macro body,
so the identifiers the fixture macros generate stay a review item, on the
`SG_BENCHMARK` precedent shape `SgBenchmarkRegistrar_##fn` and
`sgBenchmarkRegistrar_##fn` (`benchmark.hpp:582-593`). A generated name
carries no leading underscore and no `__`.

**Determinism**: scripted clock deltas from `FakeScript` drive every
timing assertion; no sleep, no wall-clock bound, no PMU (FR-029).

**Deliberately untouched**: threads and thread families, timing modes,
user counters, statistics, chunked capture, JSON reports, and topology.
Each is a roadmap spec in the spec Out list, and X.2 keeps it out.

## Constitution Check

*(Post-design pass: does the design as written honor I to XI?)*

| Principle | Verdict and note |
| --- | --- |
| I | PASS. `std::span`, `std::function`, `std::vector`, and `std::pair` carry the new surface; no extension enters. |
| II | PASS. The family-argument preconditions of FR-006, the callback-state rule of R-05, and the argument index bound all run through `SG_REQUIRE`; the one unenforceable site is recorded in Complexity Tracking. |
| III | PASS. The logical views above carry the contracts, and the physical view names the file, namespace, and target of each piece. |
| IV | PASS. The header blocks and `docs/pages/harness.md` state each contract in one place. |
| V | PASS. `suite` and `caseName` take camelBack, prefix free, under N-12, `kDefaultRangeMultiplier` and `kMaxFamilySize` take the `k` prefix under N-8 on the shape of `kDefaultMinTimeNs` (`runner.cpp:22-24`), and the nine macros take the `SG_` prefix under N-3. No `.clang-tidy` key needs a new exemption. |
| VI | PASS. Nine new test executables plus the compile-fail cases cover the new branches; the coverage exclusion list stays unchanged. |
| VII | PASS. The argument view is two words of state and one index at read time; the loop body keeps the H1 statement list. |
| VIII | PASS. No gate weakens, and the version table records the `SOVERSION` rise FR-026 requires. |
| IX | PASS. Every artifact of R-DCUT lives in the feature directory. |
| X | PASS. One new source file, one new public class, and no abstraction beyond the spec's own nouns. The `args` overload pair maps the two input forms FR-001 and FR-002 name. |
| XI | PASS. The artifacts pass the prose gate at generation (FR-030). |

## Complexity Tracking

| Item | Principle | Why it is needed | Simpler alternative rejected |
| --- | --- | --- | --- |
| FR-018 names four callback-state precondition violations; the plan enforces three | II | `State::end()` is `static auto end() noexcept -> Cursor` at `benchmark.hpp:277`, so it cannot read the instance and no precondition can guard it. It returns `Cursor {0}` and touches no state, so a callback call is harmless. | Making `end()` non-static would replace an H1 signature, which FR-022 forbids, and would break every H1 loop that already compiles. |
| Two `args` overloads | X.2 | FR-001 names `args` for a literal list and FR-002 requires accepting a `std::vector<std::int64_t>` the builders return. A single `std::span` parameter would dangle for a braced list. | One `std::span` parameter: rejected for the dangling `std::initializer_list` it invites. |
| One new source file, `source/harness/family.cpp` | X.4 | Expansion, naming, and the duplicate and size checks form one concern the registry file does not own, and the H1 layout keeps one concern per file. | Folding expansion into `registry.cpp`: rejected for the mixed concern. |
