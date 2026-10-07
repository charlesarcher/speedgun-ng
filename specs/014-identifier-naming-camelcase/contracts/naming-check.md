# Contract: name check

The closing commit makes this contract true. Research D-05 is the option
source. Research D-08 places the constitution gate item in that same
commit.

## Obligation

`readability-identifier-naming` reports zero findings on the project
tree. A finding fails the build. No other static-analysis count of a
translation unit rises (FR-004).

## Configuration the closing commit writes

`WarningsAsErrors` at `.clang-tidy:20` becomes
`readability-identifier-naming`. Other checks stay on the existing
baseline. A global `WarningsAsErrors: '*'` is rejected: the 2.10.0
report measured that it fails the build on vendored findings.

`HeaderFilterRegex` is added. It reports diagnostics from
`include/speedgun-ng/` and `source/`. It does not report diagnostics
from `external/`.

Case and prefix keys:

- Types (`ClassCase`, `StructCase`, `EnumCase`, `UnionCase`,
  `TypeAliasCase`, `TypedefCase`): `CamelCase`.
- Functions and methods (the function and method keys D-05 names):
  `camelBack`.
- Macros: `MacroDefinitionCase` `UPPER_CASE`, `MacroDefinitionPrefix`
  `SG_`.
- Enumerators: `EnumConstantCase` and `ScopedEnumConstantCase`
  `UPPER_CASE`.
- Variables, parameters, and local constants: `camelBack`.
- Public members: `camelBack`, empty prefix. Private and protected
  members: `camelBack`, prefix `m_`.
- Namespaces: `lower_case`.
- Template parameters: `CamelCase`, unchanged.
- Constants (`ConstexprVariableCase`, `StaticConstantCase`,
  `GlobalConstantCase`, `ClassConstantCase`): `CamelCase` with prefix
  `k`.

`ConceptCase` is absent from the clang-tidy 23.1.1 dump. A concept
remains PascalCase under N-1. The closing commit records whether the
check diagnoses a concept. Silence from the check leaves N-1 for
concepts to review.

N-10 has no option. Review enforces it.

## Exceptions

`IgnoreMainLikeFunctions` is `true`.

`MacroDefinitionIgnoredRegexp` includes `SPEEDGUN_NG_EXPORT`.

Each protocol spelling in the specification's exception list is named
on the `IgnoredRegexp` of the kind that declares it.

Each tag object is named in `ConstexprVariableIgnoredRegexp`, and the
entry cites N-11. The implementation enumerates those objects. This
contract does not invent the list.

A `NOLINT` that remains cites the exception entry it applies. The five
suppressions FR-019 lists are removed, because the new rules make them
unnecessary.

## Proof

SC-011 plants one misnamed identifier in a header under
`include/speedgun-ng/` and one in a `.cpp` under `source/`. Each
planting fails the name-check step. Both plantings are then removed.
A planting that stays silent means `HeaderFilterRegex` is wrong, and
the closing commit stays open.
