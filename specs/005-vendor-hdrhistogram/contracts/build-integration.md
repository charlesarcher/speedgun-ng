# Contract: HdrHistogram_c + zlib Build Integration

**Feature**: `005-vendor-hdrhistogram` | [plan.md](../plan.md) | [data-model.md](../data-model.md) | [research.md](../research.md)

Two `add_subdirectory` brackets in `CMakeLists.txt` ingest the native-CMake HdrHistogram_c and zlib submodules and produce first-class static vendored targets with every exemption the spec mandates applied to them and to no other code. A zlib redirect published between the two brackets makes HdrHistogram_c's `find_package(ZLIB)` resolve to the vendored zlib. The brackets own the vendored settings; a call site invokes them and uses the resulting targets (FR-011). Verified mechanics: research R-001, R-004 through R-012.

This is the mirror of the simdjson `add_subdirectory` bracket (specs/004), with three additions recorded here: a companion dependency whose requirement one vendored library satisfies for the other (R-008), an optionless set of HdrHistogram_c install rules that only a command override neutralizes (R-010), and a zlib symbol rename that no other vendored import needed (R-009). As with 004, the exemptions are constructed explicitly because a CMake `add_subdirectory` child inherits the parent's flags, sanitizers, coverage, clang-tidy, and `BUILD_SHARED_LIBS`, while an autotools `ExternalProject` child (specs/003) inherits none.

## 1. Interface

Two internal functions, invoked once each, plus a redirect between them. They take no arguments and expose no options: every setting is a fixed constant the spec dictates. Function scope is the isolation boundary: variables set inside do not leak to the rest of the build, while the targets created persist for use outside.

```cmake
# CMakeLists.txt, beside the existing hwloc and simdjson blocks.
import_zlib()          # publishes zlibstatic (static, hidden, Z_PREFIX) + the ZLIB::ZLIB redirect
import_hdrhistogram()  # static, hidden, logging-on against the redirect, installs neutralized

function(import_zlib)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib/CMakeLists.txt")
    message(FATAL_ERROR "external/zlib is empty; run: git submodule update --init external/zlib")
  endif()
  set(CMAKE_C_FLAGS "") set(CMAKE_CXX_FLAGS "")                       # drop parent warnings / -Werror / hardening
  set(CMAKE_C_FLAGS_SANITIZE "")                                      # FR-010 (C family; zlib is C)
  set(CMAKE_C_FLAGS_COVERAGE "")
  set(CMAKE_C_CLANG_TIDY "") set(CMAKE_C_CPPCHECK "")                 # FR-009
  set(CMAKE_C_VISIBILITY_PRESET hidden)                                # FR-022 (R-009)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)                             # absorb into a shared parent (R-006)
  set(BUILD_SHARED_LIBS OFF)                                          # FR-021 (R-007)
  set(ZLIB_BUILD_SHARED OFF) set(ZLIB_BUILD_STATIC ON) set(ZLIB_INSTALL OFF)
  set(ZLIB_BUILD_TESTING OFF) set(ZLIB_PREFIX ON)                      # FR-011/FR-022 (R-004, R-009)
  add_subdirectory(external/zlib "${CMAKE_BINARY_DIR}/_zlib" EXCLUDE_FROM_ALL)
  # FR-009/FR-012: gate compiles <zlib.h> via -isystem; promote vendored include to SYSTEM.
  get_target_property(_zlib_inc zlibstatic INTERFACE_INCLUDE_DIRECTORIES)
  set_target_properties(zlibstatic PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_zlib_inc}")
endfunction()

# Publish the redirect AFTER zlibstatic exists and BEFORE HdrHistogram_c's find_package(ZLIB).
if(NOT TARGET ZLIB::ZLIB)                                              # ZLIB_BUILD_SHARED OFF => zlib's own alias is absent
  add_library(ZLIB::ZLIB INTERFACE IMPORTED GLOBAL)
  set_target_properties(ZLIB::ZLIB PROPERTIES INTERFACE_LINK_LIBRARIES zlibstatic)
endif()
# Force FindZLIB to the vendored copy; never probe the host (FR-020, R-008).
set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR}"         CACHE PATH     "" FORCE)
set(ZLIB_LIBRARY     "${zlib_BINARY_DIR}/libz${CMAKE_STATIC_LIBRARY_SUFFIX}" CACHE FILEPATH "" FORCE)
set(ZLIB_FOUND       TRUE                        CACHE BOOL     "" FORCE)

function(import_hdrhistogram)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/hdrhistogram_c/CMakeLists.txt")
    message(FATAL_ERROR "external/hdrhistogram_c is empty; run: git submodule update --init external/hdrhistogram_c")
  endif()
  set(CMAKE_C_FLAGS "") set(CMAKE_CXX_FLAGS "")
  set(CMAKE_C_FLAGS_SANITIZE "") set(CMAKE_C_FLAGS_COVERAGE "")       # FR-010
  set(CMAKE_C_CLANG_TIDY "") set(CMAKE_C_CPPCHECK "")                 # FR-009
  set(CMAKE_C_VISIBILITY_PRESET hidden)                                # FR-017 (R-009)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(BUILD_SHARED_LIBS OFF)                                          # FR-007 (R-007)
  set(HDR_HISTOGRAM_BUILD_SHARED OFF) set(HDR_HISTOGRAM_BUILD_STATIC ON)
  set(HDR_HISTOGRAM_INSTALL_SHARED OFF) set(HDR_HISTOGRAM_INSTALL_STATIC OFF)
  set(HDR_HISTOGRAM_BUILD_PROGRAMS OFF)                               # drops test + examples (R-004)
  # HDR_LOG_REQUIRED left at its ON default: logging kept, its zlib resolves to the redirect (R-004, R-008).
  macro(install) endmacro()                                           # neutralize the 3 unconditional rules (R-010)
  add_subdirectory(external/hdrhistogram_c "${CMAKE_BINARY_DIR}/_hdrhistogram" EXCLUDE_FROM_ALL)
  # The override is confined to this function's scope; speedgun-ng installs live in cmake/install-rules.cmake.
  get_target_property(_hdr_inc hdr_histogram_static INTERFACE_INCLUDE_DIRECTORIES)
  set_target_properties(hdr_histogram_static PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_hdr_inc}")
endfunction()
```

The exact macro-vs-function form and lifetime of the `install()` override (whether a `function` or `macro`, and confirming it does not shadow speedgun-ng's own installs defined in `cmake/install-rules.cmake` at a different scope) are the one mechanism to verify empirically at implement (quickstart section 6); research R-010 records the fallback (scratch install directories) if a generator resists the override.

### Constant table (fixed constants)

| Setting | Value | Contract | Requirement |
|---|---|---|---|
| vendored paths | `external/hdrhistogram_c`, `external/zlib` | must exist and be non-empty at configure; otherwise abort naming `git submodule update --init <path>` | FR-001, FR-001a, FR-002 |
| zlib `ZLIB_INSTALL` | `OFF` | zlib defines no install/export rule; `CMAKE_INSTALL_PREFIX` receives nothing | FR-012, R-010 |
| zlib `ZLIB_BUILD_SHARED` / `ZLIB_BUILD_TESTING` | `OFF` | only `zlibstatic`; no test subdirectory, no `libz` object | FR-011, FR-021, R-004 |
| zlib `ZLIB_PREFIX` | `ON` | public zlib symbols renamed `z_*` (`Z_PREFIX`) | FR-022, SC-011, R-009 |
| HdrHistogram_c `INSTALL_SHARED`/`INSTALL_STATIC` | `OFF` | library-target install and `install(EXPORT)` disabled | FR-012, R-004 |
| HdrHistogram_c `BUILD_PROGRAMS` | `OFF` | no test/example target and no their install rules | FR-011, R-004 |
| HdrHistogram_c `HDR_LOG_REQUIRED` | default `ON` | logging kept; its `ZLIB::ZLIB` links the redirect | FR-011, Fixed decision 6, R-004 |
| `install()` override | no-op, HdrHdr bracket scope | neutralizes the 3 unconditional HdrHistogram_c install rules | FR-012, R-010 |
| zlib redirect | parent `ZLIB::ZLIB` `INTERFACE IMPORTED GLOBAL` → `zlibstatic` | sibling-visible handle for HdrHistogram_c's `find_package(ZLIB)` | FR-020, R-008 |
| `ZLIB_INCLUDE_DIR`/`ZLIB_LIBRARY`/`ZLIB_FOUND` cache | pre-seeded to vendored paths | FindZLIB never probes the host prefix | FR-020, R-008 |
| `BUILD_SHARED_LIBS` (scoped, both) | `OFF` | each vendored build static in every configuration | FR-007, FR-021, R-007 |
| `CMAKE_C_FLAGS`/`CXX` (scoped) | blanked | parent strict warnings, `-Werror`, hardening excluded | FR-009, R-011 |
| `CMAKE_C_FLAGS_SANITIZE`/`_COVERAGE` (scoped) | blanked | no sanitizer, no `--coverage` on vendored objects | FR-010, R-011 |
| `CMAKE_C_CLANG_TIDY`/`_CPPCHECK` (scoped) | blanked | no static analysis of vendored code | FR-009, R-011 |
| `CMAKE_C_VISIBILITY_PRESET` (scoped) | `hidden` | every vendored symbol stamped `STV_HIDDEN` | FR-017, FR-022, R-009 |
| `EXCLUDE_FROM_ALL` (both) | set | vendored targets excluded from default `all` | FR-011 |
| include promotion | `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES` | gate TUs compile vendored headers via `-isystem` | FR-009, FR-014, R-012 |

### Prohibited at the call site (FR-004, FR-007, FR-020)

Any `find_package(hdr_histogram)`/`pkg_check_modules(hdr_histogram)` under any FR-019 spelling, any PUBLIC link edge, any system HdrHistogram_c fallback path, and any host zlib resolution. The call site holds: the two bracket invocations, the zlib redirect and cache pre-seed, the wrapper source registrations, `target_link_libraries(speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:hdr_histogram_static>)` and `... $<BUILD_INTERFACE:zlibstatic>`, plus the static-merge wiring. `find_package(ZLIB)` inside the HdrHistogram_c subtree is permitted: it is redirected to the vendored copy, never to the host (FR-020). The `$<BUILD_INTERFACE:...>` wraps keep the vendored names out of the exported target set (the `hwloc_vendor`/`simdjson` pattern at `CMakeLists.txt:88,155`).

## 2. Guarantees

Each holds for every configuration and every Linux preset identically.

| Guarantee | Mechanism | Requirement |
|---|---|---|
| Each vendored library is a static archive in every configuration | scoped `BUILD_SHARED_LIBS OFF` plus per-tree build switches | FR-007, FR-021, R-007 |
| Neither tree installs anything, in any configuration, including `BUILD_SHARED_LIBS=ON` | `ZLIB_INSTALL OFF`; HdrHistogram_c install switches plus the `install()` override | FR-012, R-010 |
| HdrHistogram_c's `find_package(ZLIB)` resolves to the vendored zlib, host copy selected zero times | redirect published before the HdrHdr bracket; `FindZLIB` cache pre-seed | FR-020, SC-010, R-008 |
| Vendored code carries no parent warnings, `-Werror`, sanitizer, or coverage flags | scoped blanking of the `C_`/`CXX_` flag variables before each `add_subdirectory` | FR-009, FR-010, R-011 |
| Vendored code is exempt from clang-tidy/cppcheck while speedgun-ng code stays held | scoped blanking, plus the preset header-filter already excluding `external/` | FR-009, R-011 |
| Vendored objects compiled hidden; zlib additionally `z_`-prefixed | scoped `C_VISIBILITY_PRESET hidden`; `ZLIB_PREFIX ON` | FR-017, FR-022, R-009 |
| The gate TUs compile vendored headers clean under the strict set | includes promoted to SYSTEM; gate lines remain gated | FR-009, FR-014, R-012 |
| Pristine submodule worktrees | out-of-source build dirs `_zlib`/`_hdrhistogram`; zlib generates `zconf.h` into its binary dir | SC-007, R-010 |
| Offline | no download/update/git step; the submodules are the only source | FR-004 |
| Library-only artifacts | per-tree programs/test switches off, `EXCLUDE_FROM_ALL` | FR-011 |
| Early, named guard | configure-time existence check aborts with the init command | FR-002 |

## 3. Failure modes

| Condition | Detection | Result |
|---|---|---|
| Either submodule uninitialized (missing or empty dir) | configure-time existence check | `FATAL_ERROR` naming `git submodule update --init <path>`; zero fallback paths (FR-002, FR-004) |
| Wrong vendored revision | downstream compile of a wrapper's `static_assert` | compilation fails with the expected-version diagnostic (FR-003); the bracket does not second-guess git state beyond FR-002 |
| Parent configures shared (`BUILD_SHARED_LIBS=ON`) | scoped resets confine the change to the vendored builds | both still static, install still off, `.so` resolves no `libz`/`hdr_histogram` object (R-007, R-010) |
| A system HdrHistogram_c present | no discovery call exists anywhere for it | never selected; purity scan proves zero discovery calls (FR-004, SC-008) |
| A system zlib present | `FindZLIB` cache forced to vendored paths | never selected; the configure record names the vendored path (FR-020, SC-010) |
| The `install()` override misbehaves on a generator | quickstart section 6 install-tree audit | fallback to scratch install directories (R-010); the SC-002 audit is the tripwire |

## 4. Module documentation duty

The bracket adjacent comments in `CMakeLists.txt` carry: the native-CMake ingestion invariant (not the autotools module), the exemption constants and the reason each is needed (the inheritance asymmetry, R-011), the zlib-before-HdrHistogram_c ordering and the redirect rationale (R-008), the install-neutralization mechanism (R-010), and the re-pinning pointer to the README section (FR-006). Each wrapper's header comment carries the version-tripwire and link-proof explanation, matching `source/simdjson/simdjson_gate.cpp`.
