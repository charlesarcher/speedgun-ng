// The single translation unit that includes <hdr/hdr_histogram.h>
// (FR-014). The static_assert below is the version tripwire for the
// pinned submodule (FR-003): moving external/hdrhistogram_c to
// another revision fails compilation here, and re-pinning bumps the
// assertion, see the re-pinning section of README.md. The pointer
// below is the object-level link proof that keeps the vendored
// archive a real dependency (FR-007, SC-009).

#include <hdr/hdr_histogram.h>
#include <hdr/hdr_histogram_version.h>

#include <string_view>

// HDR_HISTOGRAM_VERSION lives only in hdr/hdr_histogram_version.h:
// the single string macro is the version identifier HdrHistogram_c
// exposes, so a std::string_view comparison is the compile-time
// reading (R-002).
static_assert(std::string_view{HDR_HISTOGRAM_VERSION} == "0.11.10",
              "vendored HdrHistogram_c is not the pinned "
              "HdrHistogram_c 0.11.10: re-pin external/hdrhistogram_c "
              "or bump this assertion (README re-pinning)");

namespace
{
// A function address, a link-time constant resolved to the symbol
// hdr_alloc. constinit forbids any startup initializer; this file
// carries zero runtime lines. gnu::used keeps this static alive under
// the coverage flag set, which otherwise dead-strips it and the nm
// proof loses the U reference (GCC/Clang; MSVC has no coverage build
// here).
#if defined(__GNUC__)
[[gnu::used]]
#endif
[[maybe_unused]] constinit auto const link_proof = &hdr_alloc;
}  // namespace
