// positive/static_assert_constraint.cpp
// Positive compile test for FR-022: compile-time-evaluable constraints
// must be expressed via static_assert (or concept/constexpr validator).
// Such constraints are active in every configuration at zero runtime cost.
// No runtime SG_* form is used (see negative_runtime_only_constraint.cpp for
// the forbidden form).
//
// This TU must compile cleanly.

#include <speedgun-ng/dbc.hpp>

auto main() -> int
{
  constexpr bool kConstraint = sizeof(int) == 4;  // NOLINT(readability-identifier-naming)
  static_assert(kConstraint, "int size constraint via static_assert (FR-022)");
  return 0;
}
