# Contract: fixtures

`sg::Fixture` and the seven fixture macros complete the F-2
registration surface in `include/speedgun-ng/benchmark.hpp`. The
macros carry the definition, registration, and naming behavior of the
cited revision, `google/benchmark` `main` at `e662de9a`
(`registration.h:121-163`).

## The fixture base (FR-015)

```cpp
class SPEEDGUN_NG_EXPORT Fixture
{
public:
  virtual ~Fixture();
  virtual auto setUp(State& state) -> void;    // empty default
  virtual auto tearDown(State& state) -> void; // empty default
};
```

| Rule | Requirement |
| --- | --- |
| `sg::Fixture` shall carry virtual `setUp(State&)` and `tearDown(State&)` with empty defaults. | FR-015 |
| A user class derives from `sg::Fixture` and overrides the pair. | FR-015 |
| The state handed to the pair follows the callback-state rule of `callbacks-and-state.md` (R-05). | FR-017, FR-018 |

## The fixture macros (FR-016)

```cpp
#define SG_BENCHMARK_F(FixtureClass, Method)                      // define and register
#define SG_BENCHMARK_DEFINE_F(FixtureClass, Method)               // define only
#define SG_BENCHMARK_REGISTER_F(FixtureClass, Method)             // register a definition
#define SG_BENCHMARK_TEMPLATE_F(FixtureClass, Method, ...)        // define and register, types ...
#define SG_BENCHMARK_TEMPLATE_DEFINE_F(FixtureClass, Method, ...) // define only, types ...
#define SG_BENCHMARK_TEMPLATE_METHOD_F(FixtureClass, Method)      // method of an instantiated fixture
#define SG_BENCHMARK_TEMPLATE_INSTANTIATE_F(FixtureClass, ...)    // instantiate over a type list
```

| Rule | Requirement |
| --- | --- |
| The seven macros shall exist under the `SG_` prefix with the behavior of `BENCHMARK_F`, `BENCHMARK_DEFINE_F`, `BENCHMARK_REGISTER_F`, `BENCHMARK_TEMPLATE_F`, `BENCHMARK_TEMPLATE_DEFINE_F`, `BENCHMARK_TEMPLATE_METHOD_F`, and `BENCHMARK_TEMPLATE_INSTANTIATE_F` at the cited revision (`registration.h:121-163`). | FR-016 |
| A fixture instance shall be named `FixtureClass/Method`; a template fixture instance shall be named `BaseClass<types>/Method` (`registration.h:121-130,154-163`, R-13). | FR-016 |
| `SG_BENCHMARK_DEFINE_F` followed by `SG_BENCHMARK_REGISTER_F` shall register the method later under the same name. | FR-016 |
| `SG_BENCHMARK_REGISTER_F` and `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F` shall expand to a namespace-scope declaration of one `sg::BenchmarkHandle` initialized from the registration call, leaving that call as the last token sequence, so a fixture family chains its family calls at the site: `SG_BENCHMARK_REGISTER_F(F, m).denseRange(1, 3);`. The fixture record then expands like any family record, one instance per argument list. | FR-016 |
| Generated identifiers shall follow the `SG_BENCHMARK` shape `sgBenchmarkRegistrar_##fn` (`benchmark.hpp`, R-13); they carry no leading underscore and no `__`. | FR-016, FR-023 |

## Timing of the pair (FR-017)

| Rule | Requirement |
| --- | --- |
| `setUp` shall run before the timed loop of each run and `tearDown` after it, once per run. | FR-017 |
| Warm-up, calibration, and measured runs each get the pair (Q-9). | FR-017 |
| The factory shall build one fixture object per run inside `sampleRun` (`source/harness/runner.cpp:317-366`), before `setUp`, and destroy it after `tearDown` (R-09). | FR-017 |
| The pair runs in the untimed region H1 FR-005 and FR-017 define; its work adds nothing to the reported time. | FR-017 |
| In one run the order is: setup callback, fixture `setUp`, callable, fixture `tearDown`, teardown callback (R-10). | FR-017, FR-018 |

## Suite and the `DISABLED_` prefix

| Rule | Requirement |
| --- | --- |
| A fixture instance shall take its fixture class name as its suite (R-08). | FR-021 |
| A fixture method named `DISABLED_x` shall run: the instance name starts with the class name, so the prefix test of R-11 does not reach it (`benchmark_register.cc:181`). | FR-020 |
