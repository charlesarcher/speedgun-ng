// The single translation unit that includes <hwloc.h> (FR-021).
// The static_assert below is the version tripwire for the pinned
// submodule (FR-003): moving external/hwloc to another revision fails
// compilation here, and re-pinning bumps the assertion, see the
// re-pinning section of README.md. The pointer below is the object-level
// link proof that keeps the vendored archive a real dependency
// (FR-007, SC-011).

#include <hwloc.h>

static_assert(HWLOC_VERSION_MAJOR == 2 && HWLOC_VERSION_MINOR == 14 &&
                  HWLOC_VERSION_RELEASE == 0,
              "vendored hwloc is not the pinned hwloc 2.14.0: re-pin "
              "external/hwloc or bump this assertion (README re-pinning)");

namespace
{
// A function address, a link-time constant resolved to the prefixed
// symbol sg_hwloc_get_api_version. constinit forbids any startup
// initializer; this file carries zero runtime lines.
[[maybe_unused]] constinit auto const link_proof = &hwloc_get_api_version;
}  // namespace
