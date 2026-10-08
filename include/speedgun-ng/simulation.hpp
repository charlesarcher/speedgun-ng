#ifndef SG_SIMULATION_HPP
#define SG_SIMULATION_HPP

/**
 * @file simulation.hpp
 * @brief The trace-start marker: the tag a tracer watches for and the
 * call that emits it (specs/011 FR-011, FR-013, FR-014, FR-019).
 */

#include <cstdint>

#include "speedgun-ng/speedgun-ng_export.hpp"

namespace sg
{

/**
 * @brief The tag an attached tracer watches for. The value is
 * `0xFACE`, and a caller passes it to the tracer as `-start_ssc_mark
 * FACE` on the tracer's command line, written hexadecimal with the
 * most-significant nibble first and no `0x` prefix.
 *
 * The value is part of the public contract because the caller supplies
 * it to the tracer, which is why it is published here rather than
 * derived inside the call (FR-013, FR-014).
 */
inline constexpr std::uint32_t kSimulationStartTag = 0xFACEU;

/**
 * @brief Emits the marker sequence an attached tracer watches for, so
 * collection begins at this point in the run.
 *
 * The marker has three properties a caller needs before placing it. It
 * has no architectural effect: it executes as a register move followed
 * by a prefixed one-byte no-operation, and it leaves no state a program
 * can observe after the call returns. A call is safe with no tracer
 * attached. It occupies two instructions and perturbs a measurement it
 * sits inside, so place it at the region boundary, outside the timed
 * window (FR-019).
 *
 * The tag's width matches the marker's immediate operand. A build
 * targeting a processor family with no marker instruction compiles,
 * links, and returns, emitting no marker; the tag stays published so a
 * caller can still pass it to a tracer (FR-018).
 *
 * \pre none
 * \post none
 */
SPEEDGUN_NG_EXPORT void simulationStart() noexcept;

}  // namespace sg

#endif  // SG_SIMULATION_HPP
