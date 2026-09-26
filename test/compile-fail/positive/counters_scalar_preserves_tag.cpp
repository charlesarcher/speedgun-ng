#include <type_traits>
#include <utility>

#include "speedgun-ng/counters.hpp"

using sg::counters::dim;
using sg::counters::expression;

// Scalar multiplication preserves the dimension tag (FR-015).
using scaled = decltype(2.0 * std::declval<const expression<dim<0, 1>>&>());
static_assert(std::is_same_v<typename scaled::dimension_tag, dim<0, 1>>,
              "scaling keeps the events dimension");
