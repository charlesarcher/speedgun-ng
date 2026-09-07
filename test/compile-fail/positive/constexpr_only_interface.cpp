// positive/constexpr_only_interface.cpp
// Positive compile test for FR-024: constexpr-only interfaces shall be
// constrained through the compile-time layer (static_assert/concept) and
// are exempt from runtime contract enforcement (no SG_* at such sites).
//
// This TU must compile cleanly (unconditional include, no #if on sites).

#include <speedgun-ng/dbc.hpp>

namespace
{

template <int N>
constexpr auto get_positive() -> int
{
  static_assert(N > 0, "N must be positive (ct constraint for constexpr interface)");
  return N;
}

}  // namespace

auto main() -> int
{
  constexpr int value = get_positive<42>();
  static_cast<void>(value);
  return 0;
}
