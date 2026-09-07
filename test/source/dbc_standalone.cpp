#include <cstdio>

#include <speedgun-ng/dbc.hpp>

auto main() -> int
{
  int value = 1;
  SG_REQUIRE_ALWAYS(value > 0, "value > 0");
  SG_REQUIRE(value > 0, "value > 0");
  std::puts("PASS");
  return 0;
}
