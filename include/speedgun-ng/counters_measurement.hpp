#ifndef SPEEDGUN_NG_COUNTERS_MEASUREMENT_HPP
#define SPEEDGUN_NG_COUNTERS_MEASUREMENT_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/speedgun-ng_export.hpp"

/**
 * @file counters_measurement.hpp
 * @brief The measurement spine: typed counters and expressions under
 * the dimension algebra, plan compile, the recorder factory and
 * handle, folds with disclosure, scope sugar, and the push counter
 * handle (FR-014..FR-035).
 *
 * The interfaces land with their stories (tasks T016, T017, T019,
 * T024, T037); this header is the reserved public surface for the
 * measurement spine.
 */

namespace sg::counters
{

// The algebra, plan, recorder, fold, scope, and push interfaces are
// declared by the tasks that implement them: T016 (counter and
// expression), T017 (plan compile), T019 (scope sugar), T024
// (recorder), T037 (push handle).

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_MEASUREMENT_HPP
