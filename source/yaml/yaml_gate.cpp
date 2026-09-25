// The single translation unit that includes <yaml-cpp/yaml.h>
// (FR-014). The version tripwire for the pinned submodule lives at
// configure time in the import_yaml_cpp bracket of CMakeLists.txt
// (FR-003, R-002): yaml-cpp 0.9.0 exposes no version macro, so the
// bracket reads the submodule's version declaration. Re-pinning:
// follow the re-pinning section of README.md. The pointer below is
// the object-level link proof that keeps the vendored archive a real
// dependency (FR-007, SC-009).

#include <yaml-cpp/yaml.h>

namespace
{
// A function address, a link-time constant resolved to the symbol
// YAML::Load. YAML::Load is an overload set, so the declared pointer
// type selects the const std::string& overload by context: no cast is
// needed (R-003). constinit forbids any startup initializer; this
// file carries zero runtime lines. gnu::used keeps this static alive
// under the coverage flag set, which otherwise dead-strips it and the
// nm proof loses the U reference (GCC/Clang; MSVC has no coverage
// build here).
#if defined(__GNUC__)
[[gnu::used]]
#endif
[[maybe_unused]] constinit YAML::Node (*const link_proof)(const std::string&) =
    &YAML::Load;
}  // namespace
