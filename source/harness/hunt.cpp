// specs/029 FR-002 to FR-005, FR-021, FR-022: the event-catalog discovery and
// the hunt run driver. T022 defines `EventCatalog` here and T026 `runHunt`,
// both public in speedgun-ng/hunt.hpp; T036 adds the two-stage merge. No time
// header enters this file: the driver reads no clock of its own.

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

#include "speedgun-ng/hunt.hpp"

#include "speedgun-ng/counters_system.hpp"

namespace sg::hunt
{

}  // namespace sg::hunt
