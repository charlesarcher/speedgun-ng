// The single translation unit that includes <simdjson.h> (FR-013).
// The static_assert below is the version tripwire for the pinned
// submodule (FR-003): moving external/simdjson to another revision
// fails compilation here, and re-pinning bumps the assertion, see the
// re-pinning section of README.md. The pointer below is the
// object-level link proof that keeps the vendored archive a real
// dependency (FR-007, SC-009).

#include <simdjson.h>

static_assert(simdjson::SIMDJSON_VERSION_MAJOR == 4 &&
                  simdjson::SIMDJSON_VERSION_MINOR == 6 &&
                  simdjson::SIMDJSON_VERSION_REVISION == 11,
              "vendored simdjson is not the pinned simdjson 4.6.11: "
              "re-pin external/simdjson or bump this assertion "
              "(README re-pinning)");

namespace
{
// A function address, a link-time constant resolved to the symbol
// simdjson::get_active_implementation. constinit forbids any startup
// initializer; this file carries zero runtime lines.
// gnu::used keeps this static alive under the coverage flag set,
// which otherwise dead-strips it and the nm proof loses the U
// reference (GCC/Clang; MSVC has no coverage build here).
#if defined(__GNUC__)
[[gnu::used]]
#endif
[[maybe_unused]] constinit auto const link_proof =
    &simdjson::get_active_implementation;
}  // namespace
