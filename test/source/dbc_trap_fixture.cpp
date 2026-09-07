// ============================================================================
// Red trap fixture (T004): proves the release-clean / always-on contrast.
//
// NOT registered as a ctest (it aborts). It is driven by:
//   - dbc_trap_checked_test (checked builds, default dev = enforce)
//   - the consumer-release CI job (ignore builds)
//
// Behavior:
//   - checked builds (enforce/quick_enforce): the semantic-gated site
//     SG_REQUIRE(false, ...) aborts BEFORE the marker is printed.
//   - ignore builds: the semantic-gated site is elided, so the marker is
//     printed, then the always-on site SG_REQUIRE_ALWAYS(false, ...) aborts
//     (FR-036: an always-on site fires in every configuration).
//
// (Under the `observe` semantic the gated site reports-and-continues rather
// than aborting; the fixture is only used under enforce/quick_enforce and
// ignore, so that case is out of scope for it.)
// ============================================================================

#include <cstdio>

#include "speedgun-ng/dbc.hpp"

auto main() -> int
{
  // Semantic-gated site: aborts in any checked build.
  SG_REQUIRE(false, "trap: gated site must abort in checked builds");

  // Reached only in an ignore build (the gated site above is elided).
  std::printf("gated-site-passed\n");
  std::fflush(stdout);

  // Always-on site: aborts in EVERY build, including ignore (FR-036).
  SG_REQUIRE_ALWAYS(false, "trap: always-on site must abort in all builds");

  // Unreachable: both sites above terminate.
  return 0;
}
