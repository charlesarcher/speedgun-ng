# Contract: registration, argument families, and instance names

`BenchmarkHandle` (`include/speedgun-ng/benchmark.hpp:450`) gains the
family calls, the free argument-list builders, and two registration
macros. `registerBenchmark` (`benchmark.hpp:556`), `SG_BENCHMARK`
(`benchmark.hpp:582`), `speedgunMain` (`benchmark.hpp:576`), the H1
setters, and the H1 `State` methods keep their signatures (FR-022).
Every family rule carries the semantics of the cited revision,
`google/benchmark` `main` at `e662de9a`.

## Family calls (FR-001, FR-004, FR-005, FR-006)

```cpp
// BenchmarkHandle members; each returns the handle for chaining (R-01)
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
```

| Rule | Requirement |
| --- | --- |
| Every family argument is a `std::int64_t`. | FR-001 |
| Each call shall expand with the semantics of the cited revision (`benchmark_register.cc:250-372`). | FR-001 |
| A family call shall mutate the one family record the handle points at (`source/harness/detail/internal.hpp:31`, R-01). | FR-001 |
| A family call shall accept a list `createRange` or `createDenseRange` builds. | FR-002 |
| `range` and `ranges` shall grow by powers of the range multiplier, default 8, with both bounds included; negatives follow `AddRange` (`benchmark_register.h:61`). | FR-004 |
| `rangeMultiplier` shall set the multiplier for later range calls of that family. | FR-004 |
| `denseRange` shall step from the low bound to the high bound inclusive, default step 1. | FR-005 |
| A multiplier below 2, a low bound above a high bound, and a `denseRange` step below 1 shall be `SG_REQUIRE` precondition violations (R-15). | FR-006 |
| An arity or label count unequal to the family arity shall be a precondition violation, as `benchmark_register.cc:315-324` checks. | FR-006 |
| A family that expands to more than 100 instances shall draw one warning naming `kMaxFamilySize`, keep its instances, and continue the run (R-14). | FR-007 |

## Free argument-list builders (FR-002)

```cpp
[[nodiscard]] auto createRange(std::int64_t low, std::int64_t high,
                               std::int64_t multiplier = kDefaultRangeMultiplier)
    -> std::vector<std::int64_t>;
[[nodiscard]] auto createDenseRange(std::int64_t low, std::int64_t high,
                                    std::int64_t step = 1)
    -> std::vector<std::int64_t>;
```

| Rule | Requirement |
| --- | --- |
| The builders shall carry the semantics of `CreateRange` and `CreateDenseRange` (`benchmark_register.cc:544,550`). | FR-002 |
| Each builder shall return a `std::vector<std::int64_t>`. | FR-002 |
| `kDefaultRangeMultiplier` is 8, the default of `kRangeMultiplier` (`benchmark_register.cc:61`); it takes the `k` prefix on the shape of `kDefaultMinTimeNs` (`source/harness/runner.cpp:22-24`, R-17). | FR-002, FR-004 |
| The `SG_REQUIRE` preconditions of FR-006 shall guard both builders (R-15). | FR-006 |

## Argument access (FR-008)

```cpp
// State members; the instance owns the storage (R-04)
[[nodiscard]] auto range(std::size_t index) const -> std::int64_t;
[[nodiscard]] auto rangeCount() const noexcept -> std::size_t;
```

| Rule | Requirement |
| --- | --- |
| `range(index)` shall return the instance argument at `index` and carry `SG_REQUIRE(index < size)` (R-04). | FR-008 |
| A read shall allocate nothing: `State` holds a `std::span<const std::int64_t>` the instance owns (R-04). | FR-008 |
| A family with no family call has one instance with zero arguments; `rangeCount()` reports 0 and `range(0)` is a precondition violation. | FR-008 |
| An index at or above the argument count shall be a precondition violation. | FR-008 |

## Registration macros (FR-013, FR-014)

```cpp
#define SG_BENCHMARK_CAPTURE(fn, captureName, ...)  // one instance, name fn/captureName
#define SG_BENCHMARK_TEMPLATE(fn, ...)              // one instance, name fn<types as written>
```

| Rule | Requirement |
| --- | --- |
| `SG_BENCHMARK_CAPTURE` shall register one function under the family name `fn/captureName`, as `BENCHMARK_CAPTURE` does (`registration.h:69-75`), and the captured values shall reach the function. | FR-013 |
| `SG_BENCHMARK_TEMPLATE` shall instantiate a function template over one or more type arguments and name the instance `fn<` plus the stringified type list plus `>`, as `BENCHMARK_TEMPLATE` does (`registration.h:101-107`). | FR-014 |
| The stringification shall carry the type arguments exactly as written at the macro site (R-13). | FR-014 |
| Generated identifiers shall follow the `SG_BENCHMARK` shape `SgBenchmarkRegistrar_##fn` (`benchmark.hpp:582-593`, R-13). | FR-013, FR-014 |

## Expansion and instance names (FR-003, FR-010, FR-012)

| Rule | Requirement |
| --- | --- |
| `expandRegistry()` shall run at the start of `speedgunMain` (`source/harness/cli.cpp:142`), before the filter and the first run (R-03). | FR-003 |
| No expansion work shall happen during a run. | FR-003 |
| Each instance name shall start with the family name and add one `/`-joined segment per argument (R-02). | FR-010 |
| A segment shall carry the `argName` label as `label:value` where one is set (`benchmark_api_internal.cc:34-51`). | FR-010 |
| Two instances with one name shall be a recoverable error found at expansion: one line on the standard error stream naming both names, the earlier instance stays, the later instance does not run, every other instance runs, and the exit status stays as H1 fixes it (R-03). | FR-012 |
| The family-size warning and the duplicate-instance check shall live in `expandRegistry()` (R-03). | FR-007, FR-012 |
