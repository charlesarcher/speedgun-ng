// ============================================================================
// Trap fixture for counters misuse (T023): proves contract aborts.
//
// NOT registered as a ctest (it aborts). It is driven by:
//   - counters_trap_checked_test (checked builds, default dev = enforce)
//   - the consumer-release CI job (ignore builds)
//
// Behavior per mode (argv[1]):
//   - metric-before-finish and fold-range are semantic-gated sites:
//     they abort in checked builds, so their markers stay absent; under
//     ignore they survive and print their markers.
//   - overrun is an SG_REQUIRE_ALWAYS site (FR-027 memory safety is
//     never semantic-gated): it aborts in EVERY configuration, marker
//     absent everywhere.
// A marker "counters-trap-survived-<mode>" printed after the violating
// call means the violation was NOT caught.
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string_view>

#include "speedgun-ng/counters.hpp"

namespace
{

using sg::counters::compile;
using sg::counters::dim;
using sg::counters::fake_provider;
using sg::counters::system;

using events = dim<0, 1>;

auto setup() -> sg::counters::plan
{
  auto provider = std::make_unique<fake_provider>();
  provider->add_object("package-1/core-3", "cpu3", "core", "third core");
  provider->add_counter(
      "package-1/core-3", "cycles", "ops", "core cycles elapsed");
  provider->add_counter(
      "package-1/core-3", "instructions", "ops", "instructions retired");
  provider->set_points("package-1/core-3", "cycles", {100, 300}, 200);
  provider->set_points("package-1/core-3", "instructions", {1000, 3100}, 2100);
  const auto registered =
      system::local().register_provider(std::move(provider));
  if (!registered.has_value()) {
    std::fprintf(stderr, "fixture: provider registration failed\n");
    std::exit(2);
  }
  const auto core = *system::local().object("package-1/core-3");
  const auto cycles = *core.counter<events>("cycles");
  const auto instructions = *core.counter<events>("instructions");
  auto compiled = compile(system::local(), instructions / cycles);
  if (!compiled.has_value()) {
    std::fprintf(stderr, "fixture: plan compile failed\n");
    std::exit(2);
  }
  return std::move(*compiled);
}

auto survived(std::string_view mode) -> void
{
  std::printf("counters-trap-survived-%.*s\n",
              static_cast<int>(mode.size()),
              mode.data());
  std::fflush(stdout);
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  const std::string_view mode =
      argc >= 2 ? std::string_view {argv[1]} : std::string_view {};
  auto compiled = setup();

  if (mode == "metric-before-finish") {
    const auto core = *system::local().object("package-1/core-3");
    const auto cycles = *core.counter<events>("cycles");
    const auto instructions = *core.counter<events>("instructions");
    const auto ipc = instructions / cycles;
    sg::counters::scope window {compiled};
    const auto result = window.metric(ipc);
    static_cast<void>(result);
    survived(mode);
    return 0;
  }

  if (mode == "fold-range") {
    const auto core = *system::local().object("package-1/core-3");
    const auto cycles = *core.counter<events>("cycles");
    const auto instructions = *core.counter<events>("instructions");
    const auto ipc = instructions / cycles;
    sg::counters::scope window {compiled};
    window.start();
    window.finish();
    const auto result = ipc.fold(window.view(), 1, 0);
    static_cast<void>(result);
    survived(mode);
    return 0;
  }

  if (mode == "overrun") {
    auto rec = compiled.recorder(2);
    rec.sample();
    rec.sample();
    std::fflush(stdout);
    rec.sample();
    survived(mode);
    return 0;
  }

  std::fprintf(stderr,
               "fixture: unknown mode '%.*s'\n",
               static_cast<int>(mode.size()),
               mode.data());
  return 2;
}
