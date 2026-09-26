// expect: expression addition requires identical dimension tags
#include "speedgun-ng/counters.hpp"

// Time^1 plus events^1 is a dimension violation (FR-014, US1 scenario 3).
auto add_time_to_events(
    const sg::counters::expression<sg::counters::dim<1, 0>>& time_axis,
    const sg::counters::expression<sg::counters::dim<0, 1>>& events_axis)
    -> sg::counters::expression<sg::counters::dim<1, 0>>
{
  return time_axis + events_axis;
}
