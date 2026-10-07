#include <type_traits>
#include <utility>

#include "speedgun-ng/counters.hpp"

using sg::counters::Dim;
using sg::counters::Expression;

// Scalar multiplication preserves the dimension tag (FR-015).
using scaled = decltype(2.0 * std::declval<const Expression<Dim<0, 1>>&>());
static_assert(std::is_same_v<typename scaled::DimensionTag, Dim<0, 1>>,
              "scaling keeps the events dimension");
