#include <type_traits>
#include <utility>

#include "speedgun-ng/counters.hpp"

using sg::counters::Counter;
using sg::counters::Dim;

// Quotient of identical event dimensions is dimensionless (FR-015).
using Quotient = decltype(std::declval<const Counter<Dim<0, 1>>&>()
                          / std::declval<const Counter<Dim<0, 1>>&>());
static_assert(std::is_same_v<typename Quotient::DimensionTag, Dim<0, 0>>,
              "instructions per cycle carries a dimensionless tag");
