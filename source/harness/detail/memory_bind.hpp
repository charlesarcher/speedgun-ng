#ifndef SG_HARNESS_MEMORY_BIND_HPP
#define SG_HARNESS_MEMORY_BIND_HPP

#include <cstdint>

namespace sg
{
namespace detail
{

/**
 * @brief specs/035 data-model.md: the two calls the run binds memory
 * through (FR-016, R-06). The production binder calls the hwloc membind
 * entry points by node, thread scope, inside the one hwloc translation
 * unit; a test installs a binder that records the node the call names
 * and denies the call on demand, and a null installer restores the
 * production binder, the way `installTestLayout` clears (FR-016, R-06).
 */
struct MemoryBindOps
{
  /**
   * @brief Bind the calling thread's memory policy to the node `node`
   * names (FR-010).
   *
   * \pre `node` names a node the run's resolved plan bound a context to
   * \post a true answer leaves the calling thread's memory policy the one the node's cpus carry, and a false answer leaves it the one the call found (FR-013)
   */
  auto (*bind)(std::uint32_t node) -> bool = nullptr;

  /**
   * @brief Restore the policy the caller held before its last bind
   * (FR-013).
   *
   * \pre a bind of this seam answered before the call
   * \post the calling thread's memory policy is the one the last bind found (FR-013)
   */
  auto (*restore)() -> void = nullptr;
};

}  // namespace detail
}  // namespace sg

#endif  // SG_HARNESS_MEMORY_BIND_HPP
