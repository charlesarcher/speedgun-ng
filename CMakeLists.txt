cmake_minimum_required(VERSION 3.20)

include(cmake/prelude.cmake)

project(
    speedgun-ng
    VERSION 0.1.0
    DESCRIPTION "Speedgun Next Generation - a modern C++ benchmarking framework"
    HOMEPAGE_URL "https://github.com/charlesarcher/speedgun-ng"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    speedgun-ng_speedgun-ng
    source/speedgun-ng.cpp
)
add_library(speedgun-ng::speedgun-ng ALIAS speedgun-ng_speedgun-ng)

include(GenerateExportHeader)
generate_export_header(
    speedgun-ng_speedgun-ng
    BASE_NAME speedgun-ng
    EXPORT_FILE_NAME export/speedgun-ng/speedgun-ng_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(speedgun-ng_speedgun-ng PUBLIC SPEEDGUN_NG_STATIC_DEFINE)
endif()

set_target_properties(
    speedgun-ng_speedgun-ng PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME speedgun-ng
    OUTPUT_NAME speedgun-ng
)

target_include_directories(
    speedgun-ng_speedgun-ng ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    speedgun-ng_speedgun-ng SYSTEM
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(speedgun-ng_speedgun-ng PUBLIC cxx_std_23)

# Vendored hwloc ingestion. hwloc is a private internal dependency:
# the link below is PRIVATE and the imported target is never exported,
# so no consumer of speedgun-ng::speedgun-ng can observe it (FR-007).
# Re-pinning the submodule: follow the re-pinning section of README.md
# (checkout the new tag, commit the submodule pointer, bump the
# version assertion in source/hwloc/hwloc_gate.cpp) (FR-006).
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(ImportAutotoolsSubmodule)

target_sources(speedgun-ng_speedgun-ng PRIVATE source/hwloc/hwloc_gate.cpp)

import_autotools_submodule(
    NAME hwloc_vendor
    SUBMODULE_DIR external/hwloc
    BOOTSTRAP autogen.sh
    CONFIGURE_ARGS --enable-embedded-mode
        --with-hwloc-symbol-prefix=sg_
        --disable-libxml2 --disable-opencl --disable-cairo
        --disable-cuda --disable-levelzero --disable-nvml
        --disable-rsmi --disable-libudev --disable-pci
        --disable-plugin-dlopen --disable-plugin-ltdl
    ARCHIVE hwloc/libhwloc_embedded.a
    MERGE_INTO speedgun-ng_speedgun-ng
)

# BUILD_INTERFACE keeps the vendored target out of the exported
# package files: the merged members ship inside libspeedgun-ng.a, so
# consumers must never resolve the name hwloc_vendor (privacy A2).
target_link_libraries(speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:hwloc_vendor>)

# Vendored simdjson ingestion. simdjson is a private internal
# dependency with a native CMake build, ingested through
# add_subdirectory below. A CMake child inherits the parent's compile
# flags, sanitizer and coverage instrumentation, clang-tidy and
# cppcheck; an autotools ExternalProject child inherits none of them.
# The exemptions FR-009 and FR-010 are therefore set explicitly, and
# the function scope keeps them off the rest of the build
# (research R-006). The link added beside this bracket is PRIVATE and
# BUILD_INTERFACE only: consumers of speedgun-ng::speedgun-ng observe
# nothing of simdjson (FR-007). Re-pinning the submodule: follow the
# re-pinning section of README.md (checkout the new tag, commit the
# submodule pointer, bump the version assertion in
# source/simdjson/simdjson_gate.cpp) (FR-006).
function(import_simdjson)
  # FR-002: abort configure with the init command, no fallback path.
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/simdjson/CMakeLists.txt")
    message(
        FATAL_ERROR
        "external/simdjson is uninitialized; run: "
        "git submodule update --init external/simdjson"
    )
  endif()
  # FR-009/FR-010: the vendored compile carries no parent warning
  # set, no warnings-as-errors promotion, no sanitizer or coverage
  # instrumentation, no clang-tidy, no cppcheck. FR-016: hidden
  # symbols baked in at compile time (R-007). FR-007: a static
  # archive in every configuration (R-005). FR-014: simdjson defines
  # no install rule (R-004). SIMDJSON_DEVELOPER_MODE stays unset: a
  # subproject is library-only without it (R-004).
  set(CMAKE_CXX_FLAGS "")
  set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
  set(CMAKE_CXX_FLAGS_SANITIZE "")
  set(CMAKE_CXX_FLAGS_COVERAGE "")
  set(CMAKE_CXX_CLANG_TIDY "")
  set(CMAKE_CXX_CPPCHECK "")
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  # The shared configuration absorbs the static vendored archive at
  # link time, so every vendored object must be position-independent
  # (SC-004); the property covers all vendored targets and is inert
  # for a static parent.
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(BUILD_SHARED_LIBS OFF)
  set(SIMDJSON_INSTALL OFF)
  add_subdirectory(
      external/simdjson "${CMAKE_BINARY_DIR}/_simdjson" EXCLUDE_FROM_ALL
  )
  # FR-013/R-008: the wrapper compiles <simdjson.h> with -isystem, so
  # header diagnostics stay out of the strict set while the wrapper's
  # own lines stay gated.
  get_target_property(_simdjson_inc simdjson INTERFACE_INCLUDE_DIRECTORIES)
  set_target_properties(
      simdjson PROPERTIES
      INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_simdjson_inc}"
  )
endfunction()

import_simdjson()

# The wrapper is the only TU that includes <simdjson.h> (FR-013); its
# static_assert is the version tripwire and its link-proof reference
# is the object-level proof (FR-003, FR-007). BUILD_INTERFACE keeps
# the vendored name out of the exported package files: the merged
# members ship inside libspeedgun-ng.a, so consumers must never
# resolve the name simdjson::simdjson (privacy A2).
target_sources(speedgun-ng_speedgun-ng PRIVATE source/simdjson/simdjson_gate.cpp)
target_link_libraries(speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:simdjson::simdjson>)

# Static builds merge the vendored members into libspeedgun-ng.a so
# the installed archive is self-contained and the gate reference
# resolves in-archive (R-009, SC-009); shared configurations absorb
# the private link at link time and skip the merge. The mechanism
# lives in the module: the archiver step belongs out of this call
# site (specs/003 SC-010).
include(cmake/SimdjsonArchiveMerge.cmake)
simdjson_archive_merge(speedgun-ng_speedgun-ng)

include(cmake/dbc.cmake)

# ---- Install rules ----

# Load order is load-bearing (specs/005 R-010): the HdrHistogram_c
# ingestion below defines an install() override, and a CMake command
# override is global and permanent from its point of definition
# (measured at implement: neither function scope nor a wrapper
# directory confines it). Every speedgun-ng install rule must be
# registered before that override exists. The install-tree audit of
# specs/005 quickstart section 6 and the downstream consumer test are
# the empirical guards of this ordering.

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# Vendored zlib ingestion. zlib is the companion dependency that
# HdrHistogram_c's logging component requires (specs/005 Fixed
# decision 6), ingested through add_subdirectory below, ahead of the
# HdrHistogram_c bracket. It is a private internal dependency: the
# link added further down is PRIVATE and the imported target is
# never exported, so no consumer of speedgun-ng::speedgun-ng can
# observe it (FR-021). The inheritance asymmetry the simdjson
# bracket above records applies to this native-CMake child the same
# way (specs/005 R-011), so the exemptions FR-009 and FR-010 are set
# explicitly and the function scope keeps them off the rest of the
# build. Re-pinning the submodule: follow the re-pinning section of
# README.md (checkout the new tag, commit the submodule pointer,
# bump the version assertion in source/zlib/zlib_gate.cpp) (FR-006).
function(import_zlib)
  # FR-002: abort configure with the init command, no fallback path.
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib/CMakeLists.txt")
    message(
        FATAL_ERROR
        "external/zlib is uninitialized; run: "
        "git submodule update --init external/zlib"
    )
  endif()
  # FR-009/FR-010: the vendored compile carries no parent warning
  # set, no warnings-as-errors promotion, no sanitizer or coverage
  # instrumentation, no clang-tidy, no cppcheck (R-011). FR-022:
  # hidden symbols baked in at compile time, plus the Z_PREFIX
  # rename of every public zlib symbol (R-009). FR-021: a static
  # archive in every configuration (R-007). FR-012: ZLIB_INSTALL
  # OFF removes every zlib install rule (R-010). FR-011: no test
  # subdirectory, no shared target (R-004).
  set(CMAKE_C_FLAGS "")
  set(CMAKE_CXX_FLAGS "")
  set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
  set(CMAKE_C_FLAGS_SANITIZE "")
  set(CMAKE_C_FLAGS_COVERAGE "")
  set(CMAKE_C_CLANG_TIDY "")
  set(CMAKE_C_CPPCHECK "")
  set(CMAKE_C_VISIBILITY_PRESET hidden)
  # The shared configuration absorbs the static vendored archive at
  # link time, so every vendored object must be position-independent
  # (SC-004); the property covers all vendored targets and is inert
  # for a static parent.
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(BUILD_SHARED_LIBS OFF)
  set(ZLIB_BUILD_SHARED OFF)
  set(ZLIB_BUILD_STATIC ON)
  set(ZLIB_INSTALL OFF)
  # The v1.3.1-era subtree has no ZLIB_INSTALL gate: its install
  # rules check SKIP_INSTALL_ALL instead, and their DESTINATION
  # variables default to absolute /usr/local paths that abort a
  # modern configure with the install-absolute-destination developer
  # error (specs/005 R-010). Both gate spellings are set;
  # SKIP_INSTALL_ALL is unreferenced and harmless on v1.3.2.
  set(SKIP_INSTALL_ALL ON)
  set(ZLIB_BUILD_TESTING OFF)
  set(ZLIB_PREFIX ON)
  # The vendored tree sets the real Z_PREFIX variable from ZLIB_PREFIX
  # only on the first configure of a build directory (the
  # ZLIB_CONF_WRITTEN guard skips it later), so a plain ZLIB_PREFIX
  # ON silently drops to #undef in the generated zconf.h on every
  # reconfigure. Setting Z_PREFIX alongside it keeps the rename in
  # the generated header forever: the header itself propagates the
  # renamed prototypes to every consumer through the target's include
  # interface (R-009).
  set(Z_PREFIX ON)
  # Silence the warn-uninitialized configure option here: the
  # vendored tree initializes zlib_static_suffix only on its WIN32
  # branch,
  # where the set there takes precedence; the empty suffix matches
  # the libz.a artifact the redirect cache pre-seed records. The
  # subtree's ${MSVC} reference stays unfixable from scope without
  # touching the system variable; that warning is accepted.
  set(zlib_static_suffix "")
  add_subdirectory(
      external/zlib "${CMAKE_BINARY_DIR}/_zlib" EXCLUDE_FROM_ALL
  )
  # FR-014/R-012: the gate compiles <zlib.h> with -isystem, so
  # header diagnostics stay out of the strict set while the gate's
  # own lines stay gated.
  get_target_property(_zlib_inc zlibstatic INTERFACE_INCLUDE_DIRECTORIES)
  set_target_properties(
      zlibstatic PROPERTIES
      INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_zlib_inc}"
  )
endfunction()

import_zlib()

# Publish the zlib redirect between the two brackets (specs/005
# R-008): HdrHistogram_c queries the ZLIB package through CMake's
# find module, unguarded, and the
# search must land on the vendored zlib with the host copy resolved
# zero times (FR-020). zlib's own ZLIB::ZLIB alias is insufficient:
# it is directory-scoped (invisible to the HdrHistogram_c sibling)
# and absent with ZLIB_BUILD_SHARED OFF, so the handle is a
# parent-scope IMPORTED GLOBAL interface carrying zlibstatic's usage
# requirements, the source zlib.h and the generated zconf.h among
# them. The cache pre-seed forces FindZLIB's find_path and
# find_library to the vendored values: both are no-ops when their
# cache variable is set, so FindZLIB never probes a host prefix. The
# paths are written out from the add_subdirectory call above;
# libz${CMAKE_STATIC_LIBRARY_SUFFIX} is zlibstatic's artifact name on
# the enforced platforms.
if(NOT TARGET ZLIB::ZLIB)
  add_library(ZLIB::ZLIB INTERFACE IMPORTED GLOBAL)
  set_target_properties(
      ZLIB::ZLIB PROPERTIES INTERFACE_LINK_LIBRARIES zlibstatic
  )
endif()
set(ZLIB_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib"
    CACHE PATH "" FORCE
)
set(ZLIB_LIBRARY
    "${CMAKE_BINARY_DIR}/_zlib/libz${CMAKE_STATIC_LIBRARY_SUFFIX}"
    CACHE FILEPATH "" FORCE
)
set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)

# Vendored HdrHistogram_c ingestion. HdrHistogram_c is a private
# internal dependency with a native CMake build, ingested through
# add_subdirectory below, after the zlib bracket and its redirect:
# the library's logging component resolves zlib through the redirect
# above (FR-020, R-008), and the ordering zlib-then-HdrHistogram_c is
# load-bearing. The inheritance asymmetry the simdjson bracket
# records applies the same way (specs/005 R-011). The link added
# beside this bracket is PRIVATE and BUILD_INTERFACE only: consumers
# of speedgun-ng::speedgun-ng observe nothing of HdrHistogram_c
# (FR-007). Re-pinning the submodule: follow the re-pinning section
# of README.md (checkout the new tag, commit the submodule pointer,
# bump the version assertion in
# source/hdrhistogram/hdrhistogram_gate.cpp) (FR-006).
function(import_hdrhistogram)
  # FR-002: abort configure with the init command, no fallback path.
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/hdrhistogram_c/CMakeLists.txt")
    message(
        FATAL_ERROR
        "external/hdrhistogram_c is uninitialized; run: "
        "git submodule update --init external/hdrhistogram_c"
    )
  endif()
  # FR-009/FR-010: the vendored compile carries no parent warning
  # set, no warnings-as-errors promotion, no sanitizer or coverage
  # instrumentation, no clang-tidy, no cppcheck (R-011). FR-017:
  # hidden symbols baked in at compile time; HdrHistogram_c has no
  # prefix mechanism, so hidden visibility plus the static absorption
  # close the export surface (R-009). FR-007: a static archive in
  # every configuration (R-007). FR-011: no shared target, no
  # install rule, no test or example programs (R-004).
  # HDR_LOG_REQUIRED stays at its ON default: logging is kept, and
  # its zlib requirement resolves to the redirect above (Fixed
  # decision 6).
  # The vendored subtree declares cmake_minimum_required(VERSION
  # 3.12), below CMP0077's 3.13 introduction: without the default,
  # its option() calls clear the normal variables below and the
  # policy warning escalates to a configure error. The default
  # forces NEW inside the subtree so its option() calls honor the
  # scoped exemptions (R-004).
  set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
  # Silence the warn-uninitialized configure option here: the
  # vendored CMakeLists references HDR_HISTOGRAM_PUBLIC_HEADERS
  # from the sibling include/ scope, invisible here; its expansion
  # is empty
  # either way (the headers are never compiled). The subtree's own
  # ${WIN32} reference and its unconditional include(CPack) stay
  # unfixable from scope (the system variable must not be
  # overridden; the CPack re-include is a no-op with a warning, the
  # speedgun-ng config was included first); those warnings are
  # accepted.
  set(HDR_HISTOGRAM_PUBLIC_HEADERS "")
  # Z_PREFIX for the vendored compile of this subtree (R-009):
  # zlib.h includes <zconf.h> (the generated, renamed header) only
  # under its private ZLIB_BUILD switch; a consumer's quotes-include
  # lands on the vanilla source-tree zconf.h. Compiling these sources
  # with -DZ_PREFIX activates the rename macros there, so
  # hdr_histogram_log.c calls z_inflate and z_deflate, matching the
  # vendored zlibstatic archive symbols (the bare names never exist,
  # FR-022).
  set(CMAKE_C_FLAGS "-DZ_PREFIX")
  set(CMAKE_CXX_FLAGS "")
  set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
  set(CMAKE_C_FLAGS_SANITIZE "")
  set(CMAKE_C_FLAGS_COVERAGE "")
  set(CMAKE_C_CLANG_TIDY "")
  set(CMAKE_C_CPPCHECK "")
  set(CMAKE_C_VISIBILITY_PRESET hidden)
  # The shared configuration absorbs the static vendored archive at
  # link time, so every vendored object must be position-independent
  # (SC-004); the property covers all vendored targets and is inert
  # for a static parent.
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(BUILD_SHARED_LIBS OFF)
  set(HDR_HISTOGRAM_BUILD_SHARED OFF)
  set(HDR_HISTOGRAM_BUILD_STATIC ON)
  set(HDR_HISTOGRAM_INSTALL_SHARED OFF)
  set(HDR_HISTOGRAM_INSTALL_STATIC OFF)
  set(HDR_HISTOGRAM_BUILD_PROGRAMS OFF)
  # FR-012/R-010: three HdrHistogram_c install rules are
  # unconditional, the package-config pair, the .pc file, and the
  # include/hdr headers; no option disables them. This override
  # neutralizes them. A CMake command override is global and
  # permanent from its point of definition (measured at implement),
  # so all speedgun-ng install rules are registered above this
  # function's call site (cmake/install-rules.cmake), and the
  # override swallows only the HdrHistogram_c rules: every install()
  # call names hdr_histogram files or the include/hdr destination
  # there. Any other install() call reaching this override aborts
  # configure loudly; no rule vanishes silently.
  macro(install)
    set(_hdr_ingest_rule "${ARGV};${ARGN}")
    if(NOT _hdr_ingest_rule MATCHES "[/_]hdr")
      message(
          FATAL_ERROR
          "install() rule reached the HdrHistogram_c ingestion "
          "override (specs/005 R-010): the override is global from "
          "its definition point; register new speedgun-ng install "
          "rules above the import_hdrhistogram() call site"
      )
    endif()
  endmacro()
  add_subdirectory(
      external/hdrhistogram_c
      "${CMAKE_BINARY_DIR}/_hdrhistogram" EXCLUDE_FROM_ALL
  )
  # FR-014/R-012: the wrapper compiles <hdr/hdr_histogram.h> with
  # -isystem, so header diagnostics stay out of the strict set while
  # the wrapper's own lines stay gated.
  get_target_property(
      _hdr_inc hdr_histogram_static INTERFACE_INCLUDE_DIRECTORIES
  )
  set_target_properties(
      hdr_histogram_static PROPERTIES
      INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_hdr_inc}"
  )
endfunction()

import_hdrhistogram()

# The two gate translation units are the only TUs that include
# <hdr/hdr_histogram.h> and <zlib.h> (FR-014); each static_assert is
# the version tripwire and each link-proof reference is the
# object-level proof (FR-003, FR-007, FR-021). BUILD_INTERFACE keeps
# the vendored names out of the exported package files: the merged
# members ship inside libspeedgun-ng.a, so consumers must never
# resolve the names hdr_histogram_static or zlibstatic (privacy
# A2).
target_sources(
    speedgun-ng_speedgun-ng PRIVATE source/hdrhistogram/hdrhistogram_gate.cpp
)
target_link_libraries(
    speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:hdr_histogram_static>
)
target_sources(speedgun-ng_speedgun-ng PRIVATE source/zlib/zlib_gate.cpp)
target_link_libraries(
    speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:zlibstatic>
)
# The gate TU compiles <zlib.h> outside the vendored tree, where the
# quotes-include lands on the vanilla source-tree zconf.h: activating
# Z_PREFIX at this TU applies the header's documented rename, so the
# proof reference is the archive symbol z_zlibVersion (R-009). The
# define is confined to this file; the rest of the library carries no
# zlib macros.
set_property(
    SOURCE source/zlib/zlib_gate.cpp
    PROPERTY COMPILE_DEFINITIONS Z_PREFIX
)

# Static builds merge the two vendored archives into libspeedgun-ng.a
# beside the simdjson merge: the module steps compose, each rebuilding
# the archive from the target's current members plus its own vendored
# ones (specs/005 R-006, SC-009, SC-010). Shared configurations skip
# the merges; the PRIVATE links plus the hidden objects absorb both
# archives at link time.
include(cmake/VendoredArchiveMerge.cmake)
vendored_archive_merge(
    speedgun-ng_speedgun-ng hdr_histogram_static zlibstatic
)

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${speedgun-ng_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT speedgun-ng_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of speedgun-ng"
  )
endif()

include(cmake/dbc-gate.cmake)
include(cmake/dev-mode.cmake)
