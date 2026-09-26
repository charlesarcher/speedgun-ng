#ifndef SPEEDGUN_NG_COUNTERS_SYSTEM_HPP
#define SPEEDGUN_NG_COUNTERS_SYSTEM_HPP

#include <memory>
#include <string>
#include <string_view>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/speedgun-ng_export.hpp"

/**
 * @file counters_system.hpp
 * @brief The system handle: the tree of named countable objects, the
 * merged catalog, provider registration with the open boundary, path
 * and alias resolution, structural selection, and near-miss
 * diagnostics (FR-001..FR-009).
 *
 * The interfaces land with their stories (tasks T015, T030, T031);
 * this header is the reserved public surface for the system handle.
 */

namespace sg::counters
{

// The system, object, selection, and resolution interfaces are
// declared by the tasks that implement them: T015 (system handle,
// machine root, resolution), T030 (object tree), T031 (alias
// resolution and selection).

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_SYSTEM_HPP
