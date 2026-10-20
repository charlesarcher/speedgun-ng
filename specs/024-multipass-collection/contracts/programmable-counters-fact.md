# Contract: The programmable-counter limit fact

**Feature**: 024-multipass-collection | **Data-model entity**: E-02 |
**Research**: D-01, D-02, D-03, D-04 | **Implements**: FR-005, FR-006

How many programmable counters one context can hold open at once, published
by each event source that can answer. The pass planner
(`source/harness/detail/pass_plan.hpp`, internal) is this fact's only reader
in the library's users today; the harness reads it through this surface and
nowhere else (the dogfood rule the 022 plan D-01 states).

## Public API

```cpp
struct ObjectSeed
{
  std::string_view kind;
  std::string_view path;
  std::string_view alias;
  std::string_view description;
  std::vector<CatalogSeed> entries;
  std::optional<std::size_t> programmableCounters;  // new, trailing
};

class Object
{
public:
  [[nodiscard]] auto programmableCounters() const noexcept
      -> std::optional<std::size_t>;
  // ... existing surface unchanged: name, kind, description, parent,
  // counters, children, counter<D>(name).
};

class FakeProvider final : public ProviderIface
{
public:
  auto setProgrammableCounters(std::string_view objectPath,
                               std::size_t limit) -> FakeProvider&;
  // ... existing surface unchanged: addObject, addCounter, setPoints,
  // setGapActions, readActions.
};
```

## Contract

| Name | Contract |
|---|---|
| `ObjectSeed::programmableCounters` | The provider's answer for this object, published at `enumerate`. An omitted member default-constructs to no value, so every existing provider keeps its published meaning untouched. `\pre` none; `\post` a value names a positive count (FR-005). |
| `Object::programmableCounters()` | `\pre` none. `\post` answers the published value of this object's event source; no value means the source cannot answer and the planner limits nothing for its leaves (FR-005). |
| `setProgrammableCounters(objectPath, limit)` | Scripts the answer of the named object for the next system that opens this fake. `\pre` `limit > 0` and the call precedes `System::open`, the rule every scripted fake fact keeps. `\post` the object publishes that limit (FR-006). A path the fake owns no object for is a contract violation and traps: a script that names nothing would silently test nothing (constitution II). |

The fake's scripted limit reaches the planner and nothing else: the fake
still opens any leaf set it is asked to, exactly as it does today, because
splitting is the planner's job (data-model E-03).

## Source of the real value

The `perf` PMU provider answers from CPUID leaf 0AH read at provider
construction (research D-01): version `EAX[7:0]`, programmable count
`EAX[15:8]`. The answer exists when version and count are both nonzero, and
equals the count; every other read publishes nothing. The decode is the pure
function `programmableCountersFromCpuid` (internal header, tested with
synthetic registers; research D-04); no branch of the probe depends on the
host's hardware (FR-032). Measured on the development host `corsairone` on
2026-10-20: `EAX=0x07300404` gives version 4 and count 4.

| Source | Answer |
|---|---|
| `perf` PMU objects (`cpu`, `cpu_core`, `cpu_atom`, `cpu_fixed`) on a host whose leaf answers | the count |
| `perf` PMU objects on a host whose leaf reads version 0 or count 0 | none |
| clock objects (`machine/*`), user objects, uncore and other device objects | none: they consume no programmable core counters (D-4) |
| fake objects | the scripted value when `setProgrammableCounters` named them; none otherwise |

## Worked example

```cpp
sg::counters::System sys;
sys.registerProvider(sg::counters::FakeProvider {}, "fake");
// (a FakeProvider scripted with .setProgrammableCounters("cpu", 4))
sys.open();
auto cpu = sys.object("cpu");
REQUIRE(cpu->programmableCounters().has_value());
CHECK(*cpu->programmableCounters() == 4u);
CHECK(sys.object("machine")->programmableCounters() == std::nullopt);
```

## Validation rules

| Rule | Consequence | Source |
|---|---|---|
| A source that cannot answer publishes nothing | Its leaves are costed against no limit and ride every pass that needs them | FR-005 |
| Nothing publishes a zero limit | The decode answers nothing for version 0 or count 0; a zero limit would empty every pass | D-01 |
| The fact travels by publication | The harness opens no probe groups and reads no sysfs path for it; master's probe-free behavior stands | research D-01, PC-3 |
| Fake scripts are per object path | Two faked sources can carry two different limits, the two-provider test shape | US1 edge |

## Scope boundary

The fact is a count only: it names no event, no model, no
erratum, and no counter index. Deciding what the count means for a column set
is the planner's contract (data-model E-03).
