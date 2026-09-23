# Contract: simdjson Build Integration

**Feature**: `004-vendor-simdjson` | [plan.md](../plan.md) | [data-model.md](../data-model.md) | [research.md](../research.md)

The `add_subdirectory` bracket in `CMakeLists.txt` ingests the native-CMake simdjson submodule and produces a first-class static vendored target with every exemption the spec mandates applied to it and to no other code. The bracket owns the vendored settings; a call site invokes it and uses the resulting target (FR-011). Verified mechanics: research R-001, R-004 through R-009.

This is the mirror of the hwloc `ImportAutotoolsSubmodule` contract (specs/003), with the essential difference recorded in R-006: an autotools `ExternalProject` child inherits nothing from the parent, so its exemptions are structural; a CMake `add_subdirectory` child inherits the parent's flags, sanitizers, coverage, clang-tidy, and `BUILD_SHARED_LIBS`, so every exemption must be set explicitly in a scope that does not leak.

## 1. Interface

The bracket is one internal function, invoked once. It takes no arguments and exposes no options: every setting is a fixed constant the spec dictates. The function scope is the isolation boundary: variables set inside it do not leak to the rest of the build, while the `simdjson` target it creates persists for use outside.

```cmake
# CMakeLists.txt, beside the existing hwloc block.
import_simdjson()   # internal function; the definition lives in the file

# The function body, in outline (R-006):
function(import_simdjson)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/simdjson/CMakeLists.txt")
    message(FATAL_ERROR "external/simdjson is empty; run: git submodule update --init external/simdjson")
  endif()
  set(CMAKE_CXX_FLAGS "")                              # drop parent strict warnings / -Werror / hardening
  set(CMAKE_CXX_FLAGS_SANITIZE "")                     # FR-010
  set(CMAKE_CXX_FLAGS_COVERAGE "")                     # FR-010 / FR-009
  set(CMAKE_CXX_CLANG_TIDY "")                         # FR-009
  set(CMAKE_CXX_CPPCHECK "")                           # FR-009
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)              # FR-016 (R-007)
  set(BUILD_SHARED_LIBS OFF)                           # FR-007 (R-005): simdjson always static
  set(SIMDJSON_INSTALL OFF)                            # FR-014 (R-004): simdjson installs nothing
  add_subdirectory(external/simdjson "${CMAKE_BINARY_DIR}/_simdjson" EXCLUDE_FROM_ALL)
  # FR-013 / R-008: vendored header compiled as SYSTEM for the wrapper.
  get_target_property(_simdjson_inc simdjson INTERFACE_INCLUDE_DIRECTORIES)
  set_target_properties(simdjson PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_simdjson_inc}")
endfunction()
```

### Constant table (fixed constants)

| Setting | Value | Contract | Requirement |
|---|---|---|---|
| vendored path | `external/simdjson` | must exist and be non-empty at configure; otherwise abort naming `git submodule update --init external/simdjson` | FR-001, FR-002 |
| `SIMDJSON_INSTALL` | `OFF` | simdjson defines no install/export rule; `CMAKE_INSTALL_PREFIX` receives nothing | FR-014 |
| `SIMDJSON_DEVELOPER_MODE` | unset | left undefined (never declared for a subproject) → library-only branch | FR-011 |
| `BUILD_SHARED_LIBS` | `OFF` | scoped: simdjson is a static archive even when speedgun-ng is shared | FR-007, FR-016 |
| `CMAKE_CXX_FLAGS` | blanked | parent strict warnings, `-Werror`, hardening excluded from the vendored compile | FR-009 |
| `CMAKE_CXX_FLAGS_SANITIZE` | blanked | no sanitizer instrumentation of vendored objects | FR-010 |
| `CMAKE_CXX_FLAGS_COVERAGE` | blanked | no `--coverage` on vendored objects | FR-010 |
| `CMAKE_CXX_CLANG_TIDY`, `CMAKE_CXX_CPPCHECK` | blanked | no static analysis of vendored code | FR-009 |
| `CMAKE_CXX_VISIBILITY_PRESET` | `hidden` | every vendored symbol stamped `STV_HIDDEN` | FR-016 |
| `EXCLUDE_FROM_ALL` | set | vendored target excluded from the default `all` | FR-011 |
| include promotion | `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES` | wrapper compiles `<simdjson.h>` via `-isystem` | FR-009, FR-013 |

### Prohibited at the call site (FR-004, FR-007)

Any `find_package(simdjson`, `pkg_check_modules(simdjson`, PUBLIC link edge, or system fallback path. The call site holds: this invocation, the wrapper source registration, and `target_link_libraries(speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:simdjson::simdjson>)`, plus the static-merge wiring. The `$<BUILD_INTERFACE:...>` wrap keeps the vendored name out of the exported target set (the `hwloc_vendor` pattern at `CMakeLists.txt:88`).

## 2. Guarantees

Each holds for every configuration and every Linux preset identically.

| Guarantee | Mechanism | Requirement |
|---|---|---|
| simdjson is a static archive in every configuration | scoped `BUILD_SHARED_LIBS OFF` | FR-007, R-005 |
| simdjson installs nothing, in any configuration, including `BUILD_SHARED_LIBS=ON` | explicit `SIMDJSON_INSTALL OFF` (the default tracks `BUILD_SHARED_LIBS`; the shared-audit job sets that ON) | FR-014, R-004 |
| Vendored code carries no parent warnings, `-Werror`, sanitizer, or coverage flags | scoped blanking of `CMAKE_CXX_FLAGS` and the per-config flags before `add_subdirectory` | FR-009, FR-010, R-006 |
| Vendored code is exempt from clang-tidy/cppcheck while speedgun-ng code stays held | scoped blanking of `CMAKE_CXX_CLANG_TIDY`/`_CPPCHECK`, plus the header-filter excluding `external/` | FR-009, R-006/R-008 |
| Vendored objects compiled hidden | scoped `CMAKE_CXX_VISIBILITY_PRESET hidden`; nothing in simdjson forces default visibility on ELF | FR-016, R-007 |
| The wrapper compiles `<simdjson.h>` clean under the strict set | include promoted to SYSTEM; wrapper's own lines remain gated | FR-009, FR-013, R-008 |
| Pristine submodule worktree | out-of-source build dir `${CMAKE_BINARY_DIR}/_simdjson`; nothing writes into `external/simdjson` | SC-007 |
| Offline | no download/update/git step; the submodule is the only source | FR-004 |
| Library-only artifact | developer mode left unset (`if(NOT SIMDJSON_DEVELOPER_MODE) return()`), `EXCLUDE_FROM_ALL` | FR-011 |
| Early, named guard | configure-time existence check aborts with the init command | FR-002 |

## 3. Failure modes

| Condition | Detection | Result |
|---|---|---|
| Submodule uninitialized (missing or empty dir) | configure-time existence check | `FATAL_ERROR` naming `git submodule update --init external/simdjson`; zero fallback paths (FR-002, FR-004) |
| Wrong vendored revision | downstream compile of the wrapper's `static_assert` | compilation fails with the expected-version diagnostic and the enum values (FR-003); the bracket does not second-guess the git state beyond FR-002 |
| Parent configures shared (`BUILD_SHARED_LIBS=ON`) | scoped reset confines the change to simdjson | simdjson still static, install still off, `.so` resolves no `libsimdjson` object (R-004, R-005) |
| A system simdjson present | no discovery call exists anywhere | never selected; purity scan proves zero discovery calls (FR-004, SC-008) |

## 4. Module documentation duty

The bracket's adjacent comment in `CMakeLists.txt` carries: the invariant that this is a native-CMake ingestion (not the autotools module), the exemption constants and the reason each is needed (the inheritance asymmetry, R-006), and the re-pinning pointer to the README section (FR-006). The wrapper's header comment carries the version-tripwire and link-proof explanation, matching `source/hwloc/hwloc_gate.cpp`.
