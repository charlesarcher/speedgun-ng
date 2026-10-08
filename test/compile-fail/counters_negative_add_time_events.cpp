// expect: expression addition requires identical dimension tags
#include "speedgun-ng/counters.hpp"

// Time^1 plus events^1 is a dimension violation (FR-014, US1 scenario 3).
auto addTimeToEvents(
    const sg::counters::Expression<sg::counters::Dim<1, 0>>& timeAxis,
    const sg::counters::Expression<sg::counters::Dim<0, 1>>& eventsAxis)
    -> sg::counters::Expression<sg::counters::Dim<1, 0>>
{
  return timeAxis + eventsAxis;
}
