// expect: counter addition requires identical dimension tags
#include "speedgun-ng/counters.hpp"

// Bytes (events^1) plus monotonic (time^1) is a dimension violation
// (FR-014, US1 scenario 3).
auto addBytesToMonotonic(
    const sg::counters::Counter<sg::counters::Dim<0, 1>>& bytes,
    const sg::counters::Counter<sg::counters::Dim<1, 0>>& monotonic)
    -> sg::counters::Expression<sg::counters::Dim<0, 1>>
{
  return bytes + monotonic;
}
