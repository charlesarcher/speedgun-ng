#ifndef SPEEDGUN_NG_COUNTERS_HPP
#define SPEEDGUN_NG_COUNTERS_HPP

/**
 * @file counters.hpp
 * @brief The documented single include of the counters library: the
 * standalone entry point for feature 007 (FR-049, R-001).
 *
 * Include this header to compose counters, compile plans, sample, and
 * fold. It deliberately stays out of `speedgun-ng.hpp`: the standalone
 * claim stays visible in the include graph (T011).
 */

#include "speedgun-ng/counters_clock.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_pmu.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/counters_push.hpp"
#include "speedgun-ng/counters_system.hpp"

#endif  // SPEEDGUN_NG_COUNTERS_HPP
