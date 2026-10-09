#ifndef SG_BARRIER_HPP
#define SG_BARRIER_HPP

#include <atomic>
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

/**
 * @brief Keep `value` alive against the optimizer (FR-029, D-4).
 *
 * The statement names the value as an input and output operand with a
 * register-or-memory constraint and carries a memory clobber, which is
 * the form D-4 reads at the recorded revision.
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
auto doNotOptimize(T&& value) noexcept -> void
{
  asm volatile("" : "+r,m"(value) : : "memory");
}

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
