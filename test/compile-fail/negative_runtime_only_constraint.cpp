// negative_runtime_only_constraint.cpp
// Negative test for FR-023: a compile-time-evaluable constraint expressed
// only as a runtime SG_* check. This must FAIL to compile (the compile-time
// form is mandatory).
//
// platforms: gcc,clang
// expect: compile-time-evaluable constraint must use static_assert

#include <speedgun-ng/dbc.hpp>

auto main() -> int
{
  constexpr bool kConstraint =
      sizeof(int) == 4;  // NOLINT(readability-identifier-naming)
  SG_REQUIRE(kConstraint,
             "constraint");  // runtime use of ct-evaluable constraint
  return 0;
}
