#include <string>

#include "speedgun-ng/speedgun-ng.hpp"

auto main() -> int
{
  auto const exported = exported_class {};

  return std::string("speedgun-ng") == exported.name() ? 0 : 1;
}
