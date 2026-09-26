#ifndef SPEEDGUN_NG_COUNTERS_FAKE_HPP
#define SPEEDGUN_NG_COUNTERS_FAKE_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/speedgun-ng_export.hpp"

/**
 * @file counters_fake.hpp
 * @brief The shipped fake provider: hand-driven deterministic point
 * sequences over arbitrary object trees, the exactness spine of the
 * test suite (FR-036, R-009).
 *
 * The control surface lands with US1 (task T014); this header is the
 * reserved public surface for the fake provider.
 */

namespace sg::counters
{

// The fake provider control surface is declared by task T014.

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_FAKE_HPP
