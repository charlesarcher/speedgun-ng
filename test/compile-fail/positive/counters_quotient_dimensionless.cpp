#include <type_traits>
#include <utility>

#include "speedgun-ng/counters.hpp"

using sg::counters::counter;
using sg::counters::Dim;

// Quotient of identical event dimensions is dimensionless (FR-015).
using quotient = decltype(std::declval<const counter<Dim<0, 1>>&>()
                          / std::declval<const counter<Dim<0, 1>>&>());
static_assert(std::is_same_v<typename quotient::DimensionTag, Dim<0, 0>>,
              "instructions per cycle carries a dimensionless tag");
