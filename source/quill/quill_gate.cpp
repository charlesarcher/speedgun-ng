// The single translation unit that includes <quill/Backend.h> (FR-014).
// The static_asserts below are the version tripwire for the pinned
// submodule (FR-002, FR-003): quill publishes its version as compiled
// constants in that header and publishes no preprocessor macro for it,
// and its own parsed version variable is set in its own directory scope
// only, so a preprocessor check and a read of the parsed value are both
// unavailable. The versioned inline namespace independently encodes the
// major version, so the last assertion catches an upstream edit that
// bumps one and not the other (R-002).
//
// This unit holds no symbol reference to quill, and that is deliberate.
// quill is header-only: a unit that takes the address of one quill
// function emits that function and its entire inline call graph as weak
// definitions in the same unit, and emits zero undefined symbols, at
// both -O0 and -O2. The three-way archive proof the five compiled
// dependencies carry cannot pass here, because the undefined reference it
// requires is never emitted. Measured against the pinned tree, forcing
// one costs roughly 600 KB at -O2 and 2.7 MB at -O0 of never-called code
// in the shipped archive, against roughly 4 KB for the unit below. A
// benchmarking library should not carry that weight for a symbol nothing
// calls (FR-008, SC-009a, R-008). The dependency is proven at run time
// by tools/quill/quill_dependency_check.cpp instead.
//
// Re-pinning: follow the re-pinning section of README.md. Update the
// four version assertions below alongside the submodule pointer (FR-007).

#include <quill/Backend.h>

// The expected version lives in four named constants so the tripwire reads as
// a comparison against a stated intent, and so the project's analyzer gate
// reports no magic number here (Constitution VIII).
// Re-pinning means editing exactly these four.
constexpr int expected_major = 13;
constexpr int expected_minor = 0;
constexpr int expected_patch = 0;
constexpr int expected_packed = 130000;

static_assert(quill::VersionMajor == expected_major,
              "vendored quill major version changed");
static_assert(quill::VersionMinor == expected_minor,
              "vendored quill minor version changed");
static_assert(quill::VersionPatch == expected_patch,
              "vendored quill patch version changed");
static_assert(quill::Version == expected_packed,
              "vendored quill packed version changed");

// The second, independent signal: quill wraps its API in an inline
// namespace named after the major version, so a revision that bumps the
// constants without renaming the namespace, or the reverse, fails one of
// the two checks above or this one (R-002).
namespace quill_version_namespace_probe
{
constexpr bool major_namespace_is_v13 =
    requires { sizeof(quill::v13::VersionMajor); };
static_assert(major_namespace_is_v13,
              "vendored quill major namespace is not v13");
}  // namespace quill_version_namespace_probe
