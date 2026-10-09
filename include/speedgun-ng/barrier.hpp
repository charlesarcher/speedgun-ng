#ifndef SG_BARRIER_HPP
#define SG_BARRIER_HPP

#include <atomic>
#include <type_traits>
#include <utility>

/**
 * @file barrier.hpp
 * @brief The optimization barriers of the benchmark harness: a value
 * the optimizer must keep, and a store the optimizer must not reorder.
 *
 * Feature 015-benchmark-harness-core, FR-029 and D-4. Namespace `sg`.
 * The extended-assembly statement is the one compiler extension this
 * spec adds, and specs/015-benchmark-harness-core/plan.md records its
 * P2 justification under Principle I (FR-055).
 *
 * A barrier stands only where the compiler would otherwise eliminate
 * the measured work (Principle X.2, FR-030); docs/pages/barrier-rule.md
 * states the rule.
 */

namespace sg
{

#if !defined(__GNUC__) || defined(__llvm__) || defined(__INTEL_COMPILER)

/**
 * @brief Keep `value` alive against the optimizer (FR-029, D-4).
 *
 * The overloads and their constraints mirror `DoNotOptimize` of
 * google/benchmark at the revision D-4 reads, commit
 * `e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c`, `include/benchmark/utils.h`.
 * The const-reference overload that upstream marks deprecated stays out
 * (D-4). Each statement names the value as an input and output operand
 * and carries a memory clobber:
 *
 * - the lvalue and rvalue overloads outside the `__GNUC__` branch take
 *   `"+r,m"` under `__clang__` and `"+m,r"` elsewhere;
 * - in the `__GNUC__` branch, a type that is trivially copyable and no
 *   larger than a pointer takes `"+m,r"`, and any other type takes
 *   `"+m"`.
 *
 * The constraint order is the alternative order the compiler prefers.
 * Upstream puts memory first under gcc so a memory operand stays in
 * memory, and the order is a property of the generated code that no
 * runtime check can see.
 *
 * The contract of a barrier is a property of the generated code, and no
 * runtime check can see it, so this interface carries no runtime
 * `SG_ENSURE`. The deviation from Principle II is recorded in the plan's
 * Complexity Tracking, and `test/barrier_shape.sh` enforces the property
 * at -O2 in CI.
 *
 * \pre none
 * \post none
 */
template<class T>
auto doNotOptimize(T& value) noexcept -> void
{
#  ifdef __clang__
  asm volatile("" : "+r,m"(value) : : "memory");
#  else
  asm volatile("" : "+m,r"(value) : : "memory");
#  endif
}

/**
 * @brief Keep `value` alive against the optimizer (FR-029, D-4).
 *
 * The rvalue form of the rule above: same operands, the same
 * recorded deviation.
 *
 * \pre none
 * \post none
 */
template<class T>
auto doNotOptimize(T&& value) noexcept -> void
{
#  ifdef __clang__
  asm volatile("" : "+r,m"(value) : : "memory");
#  else
  asm volatile("" : "+m,r"(value) : : "memory");
#  endif
}

#elif __GNUC__ >= 5

/**
 * @brief Keep `value` alive against the optimizer (FR-029, D-4).
 *
 * The same rule and the same recorded deviation as the overload in the
 * branch above; only the operand order differs, memory first so a
 * memory operand stays in memory.
 *
 * \pre none
 * \post none
 */
/**
 * @brief Keep `value` alive against the optimizer (FR-029, D-4).
 *
 * The small trivially copyable case: the operand order puts memory
 * first, as the branch above explains.
 *
 * \pre none
 * \post none
 */
template<class T>
  requires std::is_trivially_copyable_v<T> && (sizeof(T) <= sizeof(T*))
auto doNotOptimize(T& value) noexcept -> void
{
  asm volatile("" : "+m,r"(value) : : "memory");
}

/**
 * @brief Keep `value` alive against the optimizer (FR-029, D-4).
 *
 * The case that does not fit a register: the value takes only a
 * memory operand.
 *
 * \pre none
 * \post none
 */
template<class T>
  requires(!std::is_trivially_copyable_v<T> || (sizeof(T) > sizeof(T*)))
auto doNotOptimize(T& value) noexcept -> void
{
  asm volatile("" : "+m"(value) : : "memory");
}

/**
 * @brief Keep `value` alive against the optimizer (FR-029, D-4).
 *
 * The rvalue form of the small trivially copyable case.
 *
 * \pre none
 * \post none
 */
template<class T>
  requires std::is_trivially_copyable_v<T> && (sizeof(T) <= sizeof(T*))
auto doNotOptimize(T&& value) noexcept -> void
{
  asm volatile("" : "+m,r"(value) : : "memory");
}

/**
 * @brief Keep `value` alive against the optimizer (FR-029, D-4).
 *
 * The rvalue form of the memory-only case.
 *
 * \pre none
 * \post none
 */
template<class T>
  requires(!std::is_trivially_copyable_v<T> || (sizeof(T) > sizeof(T*)))
auto doNotOptimize(T&& value) noexcept -> void
{
  asm volatile("" : "+m"(value) : : "memory");
}

#endif

/**
 * @brief Forbid the reordering of memory accesses across this point
 * (FR-029, D-4).
 *
 * The same recorded deviation applies: the property lives in the
 * generated code, and `test/barrier_shape.sh` enforces it.
 *
 * \pre none
 * \post none
 */
inline auto clobberMemory() noexcept -> void
{
  std::atomic_signal_fence(std::memory_order_acq_rel);
}

}  // namespace sg

#endif  // SG_BARRIER_HPP
