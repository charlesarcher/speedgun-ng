#ifndef SG_HARNESS_CLI_OPTIONS_HPP
#define SG_HARNESS_CLI_OPTIONS_HPP

#include <cstddef>
#include <getopt.h>

namespace sg
{

/**
 * @file cli_options.hpp
 * @brief The long option table speedgunMain scans, exposed at file
 * scope so the option table test checks the name and value uniqueness
 * every lane keeps (specs/027-jitter-probe).
 */

// The long option table speedgunMain scans with getopt_long: one row
// per option, the sentinel row of nulls last. The scanner carries no
// short letters, so the long-only options take their switch values
// above the letter range: the metric method holds 256, the compare
// family 257 to 259, and the jitter probe opens the lane block at 260
// (`--jitter-probe`).
extern const option kLongOptions[];

}

#endif
