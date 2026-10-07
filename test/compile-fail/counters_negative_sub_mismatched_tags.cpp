// expect: counter subtraction requires identical dimension tags
#include "speedgun-ng/counters.hpp"

// Cycle time (time^1) minus bytes (events^1) is a dimension violation
// (FR-014, US1 scenario 3).
auto subtract_bytes_from_monotonic(
    const sg::counters::counter<sg::counters::Dim<1, 0>>& monotonic,
    const sg::counters::counter<sg::counters::Dim<0, 1>>& bytes)
    -> sg::counters::expression<sg::counters::Dim<1, 0>>
{
  return monotonic - bytes;
}
