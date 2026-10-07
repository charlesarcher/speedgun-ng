#include <cstdio>
#include <string>

#include "../../source/counters/detail/pmu.hpp"

auto main(int argc, char** argv) -> int
{
  const std::string directory =
      argc > 1 ? argv[1] : std::string("arch/x86/skylake/");
  const auto& table = sg::counters::detail::pmu_load_table(directory);
  std::printf("embedded: %s %zu\n", directory.c_str(), table.size());
  return 0;
}
