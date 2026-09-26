// ============================================================================
// Allocation-free sampling proof (T027, FR-026).
//
// Replaces the global operator new/delete in this translation unit with
// counting wrappers, then samples in a loop with counting enabled only
// around the sample calls. Any heap allocation on the critical path
// fails the check.
// ============================================================================

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>

#include "speedgun-ng/counters.hpp"

namespace
{

std::atomic<bool> counting {false};
std::atomic<long long> allocations {0};

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    std::fprintf(stderr, "COUNTERS NOALLOC TEST FAIL: %s\n", what);
    std::exit(1);
  }
}

using sg::counters::compile;
using sg::counters::dim;
using sg::counters::fake_provider;
using sg::counters::system;

using events = dim<0, 1>;

}  // namespace

auto operator new(std::size_t size) -> void*
{
  if (counting.load(std::memory_order_relaxed)) {
    ++allocations;
  }
  void* p = std::malloc(size == 0 ? 1 : size);
  if (p == nullptr) {
    throw std::bad_alloc();
  }
  return p;
}

auto operator delete(void* p) noexcept -> void
{
  std::free(p);
}

auto operator delete(void* p, std::size_t) noexcept -> void
{
  std::free(p);
}

auto main() -> int
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
  check(registered.has_value(), "the scripted provider registers");

  const auto core = *system::local().object("package-1/core-3");
  const auto cycles = *core.counter<events>("cycles");
  const auto instructions = *core.counter<events>("instructions");
  const auto ipc = instructions / cycles;
  auto compiled = compile(system::local(), ipc);
  check(compiled.has_value(), "ipc plan compiles");

  auto rec = compiled->recorder(16, sg::counters::ring);
  check(rec.has_value(), "ring recorder accepted");

  counting.store(true);
  for (int i = 0; i < 1000; ++i) {
    rec->sample();
  }
  counting.store(false);

  check(rec->count() == 16, "ring recorder filled to capacity (FR-026)");
  check(allocations.load() == 0,
        "sampling performed zero heap allocations (FR-026)");

  const auto whole = ipc.fold(rec->view());
  check(whole.value > 0.0, "the sampled window folds");
  std::printf("counters noalloc tests passed\n");
  return 0;
}
