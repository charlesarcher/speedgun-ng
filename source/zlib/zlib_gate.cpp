// The single translation unit that includes <zlib.h> (FR-014). The
// static_assert below is the version tripwire for the pinned
// submodule (FR-003): moving external/zlib to another revision fails
// compilation here, and re-pinning bumps the assertion, see the
// re-pinning section of README.md. The pointer below is the
// object-level link proof that keeps the vendored archive a real
// dependency (FR-021, SC-010).

#include <zlib.h>

#include <string_view>

// ZLIB_VERSION is the macro FR-003 names; ZLIB_VERNUM rejects a
// patch drift with no ambiguity (no ZLIB_VER_PATCH exists, R-003).
static_assert(std::string_view{ZLIB_VERSION} == "1.3.2" &&
                  ZLIB_VERNUM == 0x1320u,
              "vendored zlib is not the pinned zlib 1.3.2: re-pin "
              "external/zlib or bump this assertion (README re-pinning)");

namespace
{
// A function address, a link-time constant resolved to the symbol
// the archive carries. Under Z_PREFIX the header renames zlibVersion
// to z_zlibVersion, matching the vendored symbol with no special
// handling here (R-005). constinit forbids any startup initializer;
// this file carries zero runtime lines. gnu::used keeps this static
// alive under the coverage flag set, which otherwise dead-strips it
// and the nm proof loses the U reference (GCC/Clang; MSVC has no
// coverage build here).
#if defined(__GNUC__)
[[gnu::used]]
#endif
[[maybe_unused]] constinit auto const link_proof = &zlibVersion;
}  // namespace
