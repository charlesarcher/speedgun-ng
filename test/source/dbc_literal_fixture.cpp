#include "speedgun-ng/dbc.hpp"

auto main() -> int
{
  char const* const kept = "check_precondition";
  return kept == nullptr ? 1 : 0;
}
