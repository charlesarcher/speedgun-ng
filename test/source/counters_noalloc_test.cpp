// ============================================================================
// Allocation-free sampling proof (T027, T108; FR-026, SC-005).
//
// Replaces the global operator new/delete in this translation unit with
// counting wrappers, then samples in a loop with counting enabled only
// around the sample calls. Any heap allocation on the critical path
// fails the check.
//
// Every standard replaceable form is replaced: plain, array, nothrow,
// and the aligned forms, each with its matching delete. Counting only
// the plain form would let an array-form or nothrow-form allocation on
// the sample path escape the count, so the whole set is what makes the
// zero-allocation claim mean anything.
// ============================================================================

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

auto note_allocation() -> void
{
  if (counting.load(std::memory_order_relaxed)) {
    ++allocations;
  }
}

auto block(const std::size_t size) -> void*
{
  void* p = std::malloc(size == 0 ? 1 : size);
  if (p == nullptr) {
    throw std::bad_alloc();
  }
  return p;
}

auto alloc(const std::size_t size) -> void*
{
  note_allocation();
  return block(size);
}

auto alloc_nothrow(const std::size_t size) noexcept -> void*
{
  note_allocation();
  return std::malloc(size == 0 ? 1 : size);
}

// The aligned forms: malloc one alignment wider than the request, keep
// the malloc'd base in the word before the aligned address, and hand the
// aligned address out. The paired delete reads the base back, so each
// block returns to malloc exactly once and the alignment survives.
auto alloc_aligned(const std::size_t size, const std::size_t align) -> void*
{
  note_allocation();
  void* base = std::malloc((size == 0 ? 1 : size) + align);
  if (base == nullptr) {
    throw std::bad_alloc();
  }
  const auto mask = static_cast<std::uintptr_t>(align) - 1;
  const auto address = reinterpret_cast<std::uintptr_t>(base);
  auto* out = reinterpret_cast<void*>((address + mask) & ~mask);
  std::memcpy(out, &base, sizeof(base));
  return out;
}

auto alloc_aligned_nothrow(const std::size_t size,
                           const std::size_t align) noexcept -> void*
{
  try {
    return alloc_aligned(size, align);
  } catch (...) {
    return nullptr;
  }
}

auto free_aligned(void* p) noexcept -> void
{
  if (p == nullptr) {
    return;
  }
  void* base = nullptr;
  std::memcpy(&base, p, sizeof(base));
  std::free(base);
}

}  // namespace

auto operator new(std::size_t size) -> void*
{
  return alloc(size);
}

auto operator new[](std::size_t size) -> void*
{
  return alloc(size);
}

auto operator new(std::size_t size, const std::nothrow_t&) noexcept -> void*
{
  return alloc_nothrow(size);
}

auto operator new[](std::size_t size, const std::nothrow_t&) noexcept -> void*
{
  return alloc_nothrow(size);
}

auto operator new(std::size_t size, std::align_val_t align) -> void*
{
  return alloc_aligned(size, static_cast<std::size_t>(align));
}

auto operator new[](std::size_t size, std::align_val_t align) -> void*
{
  return alloc_aligned(size, static_cast<std::size_t>(align));
}

auto operator new(std::size_t size,
                  std::align_val_t align,
                  const std::nothrow_t&) noexcept -> void*
{
  return alloc_aligned_nothrow(size, static_cast<std::size_t>(align));
}

auto operator new[](std::size_t size,
                    std::align_val_t align,
                    const std::nothrow_t&) noexcept -> void*
{
  return alloc_aligned_nothrow(size, static_cast<std::size_t>(align));
}

// Both delete forms are required: GCC 16 reports
// `-Wsized-deallocation` when a translation unit that allocates has only
// the unsized replacement. With the sized form present, the same
// compiler reports `-Wmismatched-new-delete` at each `free` below,
// because its warning pass attributes every allocation in the
// translation unit to the default `operator new` and never consults the
// replacement declared here. The pairing is correct: these
// allocations hand out `malloc` memory (the aligned ones keep the base
// word before the aligned address) and the deletes return it with `free`.
// The suppression covers exactly that false positive (constitution I,
// X.2).
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

auto operator delete(void* p) noexcept -> void
{
  std::free(p);
}

auto operator delete[](void* p) noexcept -> void
{
  std::free(p);
}

auto operator delete(void* p, std::size_t) noexcept -> void
{
  std::free(p);
}

auto operator delete[](void* p, std::size_t) noexcept -> void
{
  std::free(p);
}

auto operator delete(void* p, const std::nothrow_t&) noexcept -> void
{
  std::free(p);
}

auto operator delete[](void* p, const std::nothrow_t&) noexcept -> void
{
  std::free(p);
}

auto operator delete(void* p, std::align_val_t) noexcept -> void
{
  free_aligned(p);
}

auto operator delete[](void* p, std::align_val_t) noexcept -> void
{
  free_aligned(p);
}

auto operator delete(void* p, std::size_t, std::align_val_t) noexcept -> void
{
  free_aligned(p);
}

auto operator delete[](void* p, std::size_t, std::align_val_t) noexcept -> void
{
  free_aligned(p);
}

auto operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept
    -> void
{
  free_aligned(p);
}

auto operator delete[](void* p,
                       std::align_val_t,
                       const std::nothrow_t&) noexcept -> void
{
  free_aligned(p);
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

  // The counter covers every replaceable form, so arm it and take one
  // allocation of each form that is not the plain one: three
  // allocations, three counts, and the aligned one still aligned.
  counting.store(true);
  const auto array = new int[4];
  const auto nothrow = new (std::nothrow) int;
  const auto aligned = new (std::align_val_t {64}) int;
  counting.store(false);
  check(allocations.load() == 3,
        "the array, nothrow, and aligned forms are counted (SC-005)");
  check(reinterpret_cast<std::uintptr_t>(aligned) % 64 == 0,
        "the aligned form hands back 64-byte aligned storage (SC-005)");
  delete[] array;
  delete nothrow;
  delete aligned;

  const auto whole = ipc.fold(rec->view());
  check(whole.value > 0.0, "the sampled window folds");
  std::printf("counters noalloc tests passed\n");
  return 0;
}
